// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <iterator>
#include <memory>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/detail/last_error.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"

namespace winternl = hadesmem::detail::winternl;

namespace
{

std::unique_ptr<hadesmem::PatchDetour>& GetNtQuerySystemInformationDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

// TODO: Rewrite this to use templates (similar to DirectoryFileInformationEnum)
// instead of casting everywhere? Requires more boilerplate, but ends up easier
// to write and maintain.
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

  bool IsValid() const HADESMEM_DETAIL_NOEXCEPT
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
    // Check whether buffer is valid or it's the first process in the list
    // (which is always System Idle Process).
    return (buffer_->ImageName.Buffer != nullptr &&
            buffer_->ImageName.Length) ||
           buffer_->UniqueProcessId == 0;
  }

  std::wstring GetName() const
  {
    if (buffer_->UniqueProcessId == 0)
    {
      return {L"System Idle Process"};
    }

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

extern "C" NTSTATUS WINAPI NtQuerySystemInformationDetour(
  winternl::SYSTEM_INFORMATION_CLASS system_information_class,
  PVOID system_information,
  ULONG system_information_length,
  PULONG return_length) HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::LastErrorPreserver last_error;
  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%d] [%p] [%u] [%p].",
                                 system_information_class,
                                 system_information,
                                 system_information_length,
                                 return_length);
  auto& detour = GetNtQuerySystemInformationDetour();
  auto const nt_query_system_information =
    detour->GetTrampoline<decltype(&NtQuerySystemInformationDetour)>();
  last_error.Revert();
  auto const ret = nt_query_system_information(system_information_class,
                                               system_information,
                                               system_information_length,
                                               return_length);
  last_error.Update();
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
         process_info.IsValid();
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
}

void DetourNtQuerySystemInformation()
{
  hadesmem::Module const ntdll(GetThisProcess(), L"ntdll.dll");
  auto const nt_query_system_information = hadesmem::FindProcedure(
    GetThisProcess(), ntdll, "NtQuerySystemInformation");
  auto const nt_query_system_information_ptr =
    hadesmem::detail::UnionCast<void*>(nt_query_system_information);
  auto const nt_query_system_information_detour =
    hadesmem::detail::UnionCast<void*>(&NtQuerySystemInformationDetour);
  auto& detour = GetNtQuerySystemInformationDetour();
  detour.reset(new hadesmem::PatchDetour(GetThisProcess(),
                                         nt_query_system_information_ptr,
                                         nt_query_system_information_detour));
  detour->Apply();
  HADESMEM_DETAIL_TRACE_A("NtQuerySystemInformation detoured.");
}
