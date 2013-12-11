// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <string>
#include <iterator>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

// TODO: Write a test app to exercise all the hooked APIs and information
// classes.

namespace winternl = hadesmem::detail::winternl;

std::unique_ptr<hadesmem::Process> g_process;

std::unique_ptr<hadesmem::PatchDetour> g_nt_query_system_information_hk;

class SystemProcessInformationEnum
{
public:
  explicit SystemProcessInformationEnum(
    winternl::SYSTEM_INFORMATION_CLASS info_class,
    void* buffer) HADESMEM_DETAIL_NOEXCEPT
    : buffer_(GetRealBuffer(info_class, buffer)),
      prev_(nullptr),
      info_class_(info_class),
      unlinked_(false)
  {
    HADESMEM_DETAIL_ASSERT(buffer_);
    HADESMEM_DETAIL_ASSERT(
      info_class == winternl::SystemProcessInformation ||
      info_class == winternl::SystemExtendedProcessInformation ||
      info_class == winternl::SystemSessionProcessInformation ||
      info_class == winternl::SystemFullProcessInformation);
  }

  SystemProcessInformationEnum(SystemProcessInformationEnum const&) = delete;

  SystemProcessInformationEnum& operator=(SystemProcessInformationEnum const&) =
    delete;

  void Advance() HADESMEM_DETAIL_NOEXCEPT
  {
    if (!unlinked_)
    {
      prev_ = buffer_;
    }

    unlinked_ = false;

    if (buffer_->NextEntryOffset)
    {
      buffer_ = reinterpret_cast<winternl::SYSTEM_PROCESS_INFORMATION*>(
        reinterpret_cast<DWORD_PTR>(buffer_) + buffer_->NextEntryOffset);
    }
    else
    {
      buffer_ = nullptr;
    }
  }

  bool Valid() const HADESMEM_DETAIL_NOEXCEPT
  {
    return buffer_ != nullptr;
  }

  void Unlink() HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(buffer_);

    HADESMEM_DETAIL_ASSERT(!unlinked_);

    if (prev_)
    {
      if (buffer_->NextEntryOffset)
      {
        prev_->NextEntryOffset += buffer_->NextEntryOffset;
      }
      else
      {
        prev_->NextEntryOffset = 0UL;
      }

      unlinked_ = true;
    }
    else
    {
      // Unlinking the first process is unsupported.
      HADESMEM_DETAIL_ASSERT(false);
    }
  }

  bool HasName() const HADESMEM_DETAIL_NOEXCEPT
  {
    return buffer_->ImageName.Buffer != nullptr && buffer_->ImageName.Length;
  }

  // TODO: Remove dependency on std::wstring, we shouldn't need to allocate
  // and copy here. This should be a noexcept function returning a range over
  // the existing string buffer.
  std::wstring GetName() const
  {
    auto const str_end =
      std::find(buffer_->ImageName.Buffer,
                buffer_->ImageName.Buffer + buffer_->ImageName.Length,
                L'\0');
    // SystemFullProcessInformation returns the full path rather than just the
    // image name.
    if (info_class_ == winternl::SystemFullProcessInformation)
    {
      auto const name_beg =
        std::find(std::reverse_iterator<wchar_t*>(str_end),
                  std::reverse_iterator<wchar_t*>(buffer_->ImageName.Buffer),
                  L'\\');
      return {name_beg.base(), str_end};
    }
    else
    {
      return {buffer_->ImageName.Buffer, str_end};
    }
  }

private:
  winternl::SYSTEM_PROCESS_INFORMATION*
    GetRealBuffer(winternl::SYSTEM_INFORMATION_CLASS info_class,
                  void* buffer) const HADESMEM_DETAIL_NOEXCEPT
  {
    if (info_class == winternl::SystemProcessInformation ||
        info_class == winternl::SystemExtendedProcessInformation ||
        info_class == winternl::SystemFullProcessInformation)
    {
      return static_cast<winternl::SYSTEM_PROCESS_INFORMATION*>(buffer);
    }
    else if (info_class == winternl::SystemSessionProcessInformation)
    {
      return static_cast<winternl::SYSTEM_PROCESS_INFORMATION*>(
        static_cast<winternl::SYSTEM_SESSION_PROCESS_INFORMATION*>(buffer)
          ->Buffer);
    }
    else
    {
      HADESMEM_DETAIL_ASSERT(false);
      return nullptr;
    }
  }

  winternl::SYSTEM_PROCESS_INFORMATION* buffer_;
  winternl::SYSTEM_PROCESS_INFORMATION* prev_;
  winternl::SYSTEM_INFORMATION_CLASS info_class_;
  bool unlinked_;
};

// TODO: Preserve LastError value.
extern "C" NTSTATUS WINAPI NtQuerySystemInformationHk(
  winternl::SYSTEM_INFORMATION_CLASS system_information_class,
  PVOID system_information,
  ULONG system_information_length,
  PULONG return_length) HADESMEM_DETAIL_NOEXCEPT
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%d] [%p] [%u] [%p].",
                                 system_information_class,
                                 system_information,
                                 system_information_length,
                                 return_length);
  auto const nt_query_system_information =
    g_nt_query_system_information_hk
      ->GetTrampoline<decltype(&NtQuerySystemInformationHk)>();
  auto const ret = nt_query_system_information(system_information_class,
                                               system_information,
                                               system_information_length,
                                               return_length);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  // TODO: Handle SystemProcessIdInformation (88).
  if (system_information_class != winternl::SystemProcessInformation &&
      system_information_class != winternl::SystemExtendedProcessInformation &&
      system_information_class != winternl::SystemSessionProcessInformation &&
      system_information_class != winternl::SystemFullProcessInformation)
  {
    HADESMEM_DETAIL_TRACE_A("Unhandled information class.");
    return ret;
  }

  if (!NT_SUCCESS(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Trampoline returned failure.");
    return ret;
  }

  try
  {
    HADESMEM_DETAIL_TRACE_A("Enumerating processes.");
    for (SystemProcessInformationEnum process_info{system_information_class,
                                                   system_information};
         process_info.Valid();
         process_info.Advance())
    {
      if (process_info.HasName())
      {
        auto const process_name = process_info.GetName();
        HADESMEM_DETAIL_TRACE_FORMAT_W(L"Name: [%s].", process_name.c_str());
        if (process_name == L"hades.exe")
        {
          HADESMEM_DETAIL_TRACE_A("Unlinking process.");
          process_info.Unlink();
        }
      }
      else
      {
        HADESMEM_DETAIL_TRACE_A("WARNING! Invalid name.");
      }
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return ret;
}

extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Load() HADESMEM_DETAIL_NOEXCEPT
{
  try
  {
    g_process.reset(new hadesmem::Process(::GetCurrentProcessId()));
    hadesmem::Module const ntdll(*g_process, L"ntdll.dll");
    auto const nt_query_system_information =
      hadesmem::FindProcedure(*g_process, ntdll, "NtQuerySystemInformation");
    auto const nt_query_system_information_ptr =
      hadesmem::detail::UnionCast<void*>(nt_query_system_information);
    auto const nt_query_system_information_hk =
      hadesmem::detail::UnionCast<void*>(&NtQuerySystemInformationHk);
    g_nt_query_system_information_hk.reset(
      new hadesmem::PatchDetour(*g_process,
                                nt_query_system_information_ptr,
                                nt_query_system_information_hk));
    g_nt_query_system_information_hk->Apply();
    HADESMEM_DETAIL_TRACE_A("NtQuerySystemInformationHk hooked.");
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return 0;
}

// TODO: Safe unhooking and unloading.
extern "C" HADESMEM_DETAIL_DLLEXPORT DWORD_PTR Free() HADESMEM_DETAIL_NOEXCEPT
{
  return 0;
}

BOOL WINAPI
  DllMain(HINSTANCE /*instance*/, DWORD /*reason*/, LPVOID /*reserved*/)
{
  return TRUE;
}
