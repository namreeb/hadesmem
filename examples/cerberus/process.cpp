// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "process.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/recursion_protector.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "detour_ref_counter.hpp"
#include "main.hpp"

namespace winternl = hadesmem::detail::winternl;

namespace
{

std::unique_ptr<hadesmem::PatchDetour>& GetNtQuerySystemInformationDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetNtQuerySystemInformationRefCount()
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetNtCreateUserProcessDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetNtCreateUserProcessRefCount()
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

bool HasNameImpl(void* pid, UNICODE_STRING const& path) HADESMEM_DETAIL_NOEXCEPT
{
  // Check whether buffer is valid or it's the first process in the list
  // (which is always System Idle Process).
  return (path.Buffer != nullptr && path.Length) || pid == 0;
}

std::wstring GetNameImpl(winternl::SYSTEM_INFORMATION_CLASS info_class,
                         void* pid,
                         UNICODE_STRING const& path)
{
  if (pid == 0)
  {
    return {L"System Idle Process"};
  }

  auto const str_end = std::find(path.Buffer, path.Buffer + path.Length, L'\0');
  // SystemFullProcessInformation returns the full path rather than just the
  // image name.
  // SystemProcessIdInformation returns the full path in its native (NT) format.
  if (info_class == winternl::SystemFullProcessInformation ||
      info_class == winternl::SystemProcessIdInformation)
  {
    auto const name_beg =
      std::find(std::reverse_iterator<wchar_t*>(str_end),
                std::reverse_iterator<wchar_t*>(path.Buffer),
                L'\\');
    return {name_beg.base(), str_end};
  }
  else
  {
    return {path.Buffer, str_end};
  }
}

class SystemProcessInformationEnum
{
public:
  explicit SystemProcessInformationEnum(
    winternl::SYSTEM_INFORMATION_CLASS info_class,
    void* buffer,
    void* buffer_end) HADESMEM_DETAIL_NOEXCEPT
    : buffer_(GetRealBuffer(info_class, buffer)),
      buffer_end_(buffer_end),
      info_class_(info_class)
  {
    HADESMEM_DETAIL_ASSERT(buffer_);
    HADESMEM_DETAIL_ASSERT(buffer_end > buffer_);
    HADESMEM_DETAIL_ASSERT(
      info_class == winternl::SystemProcessInformation ||
      info_class == winternl::SystemExtendedProcessInformation ||
      info_class == winternl::SystemSessionProcessInformation ||
      info_class == winternl::SystemFullProcessInformation);
  }

  SystemProcessInformationEnum(SystemProcessInformationEnum const&) = delete;

  SystemProcessInformationEnum&
    operator=(SystemProcessInformationEnum const&) = delete;

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

    // Anything but first in the list
    if (prev_)
    {
      // Middle of the list
      if (buffer_->NextEntryOffset)
      {
        prev_->NextEntryOffset += buffer_->NextEntryOffset;
      }
      // Last in the list
      else
      {
        prev_->NextEntryOffset = 0UL;
      }

      unlinked_ = true;
    }
    // First in the list
    else
    {
      // Unlinking the first process is unsupported.
      HADESMEM_DETAIL_ASSERT(false);
    }
  }

  bool HasName() const HADESMEM_DETAIL_NOEXCEPT
  {
    return HasNameImpl(buffer_->UniqueProcessId, buffer_->ImageName);
  }

  std::wstring GetName() const
  {
    return GetNameImpl(
      info_class_, buffer_->UniqueProcessId, buffer_->ImageName);
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
  void* buffer_end_;
  winternl::SYSTEM_PROCESS_INFORMATION* prev_{};
  winternl::SYSTEM_INFORMATION_CLASS info_class_;
  bool unlinked_{};
};

extern "C" NTSTATUS WINAPI NtQuerySystemInformationDetour(
  winternl::SYSTEM_INFORMATION_CLASS system_information_class,
  PVOID system_information,
  ULONG system_information_length,
  PULONG return_length) HADESMEM_DETAIL_NOEXCEPT
{
  DetourRefCounter ref_count{GetNtQuerySystemInformationRefCount()};

  hadesmem::detail::LastErrorPreserver last_error_preserver;
  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%d] [%p] [%lu] [%p].",
                                 system_information_class,
                                 system_information,
                                 system_information_length,
                                 return_length);
  auto& detour = GetNtQuerySystemInformationDetour();
  auto const nt_query_system_information =
    detour->GetTrampoline<decltype(&NtQuerySystemInformationDetour)>();
  last_error_preserver.Revert();
  auto const ret = nt_query_system_information(system_information_class,
                                               system_information,
                                               system_information_length,
                                               return_length);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (system_information_class != winternl::SystemProcessInformation &&
      system_information_class != winternl::SystemExtendedProcessInformation &&
      system_information_class != winternl::SystemSessionProcessInformation &&
      system_information_class != winternl::SystemFullProcessInformation &&
      system_information_class != winternl::SystemProcessIdInformation)
  {
    HADESMEM_DETAIL_TRACE_A("Unhandled information class.");
    return ret;
  }

  if (!NT_SUCCESS(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
    return ret;
  }

  try
  {
    if (system_information_class == winternl::SystemProcessIdInformation)
    {
      auto const* const pid_info =
        static_cast<winternl::SYSTEM_PROCESS_ID_INFORMATION*>(
          system_information);
      if (HasNameImpl(pid_info->ProcessId, pid_info->ImageName))
      {
        auto const process_name = GetNameImpl(
          system_information_class, pid_info->ProcessId, pid_info->ImageName);
        HADESMEM_DETAIL_TRACE_FORMAT_W(L"Name: [%s].", process_name.c_str());
        if (process_name == L"hades.exe")
        {
          HADESMEM_DETAIL_TRACE_A("Returning failure to hide process.");
          return STATUS_INVALID_PARAMETER;
        }
      }
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Enumerating processes.");
      for (SystemProcessInformationEnum process_info{
             system_information_class, system_information,
             static_cast<std::uint8_t*>(system_information) +
               system_information_length};
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
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return ret;
}

extern "C" NTSTATUS WINAPI
  NtCreateUserProcessDetour(PHANDLE process_handle,
                            PHANDLE thread_handle,
                            ACCESS_MASK process_desired_access,
                            ACCESS_MASK thread_desired_access,
                            POBJECT_ATTRIBUTES process_object_attributes,
                            POBJECT_ATTRIBUTES thread_object_attributes,
                            ULONG process_flags,
                            ULONG thread_flags,
                            PRTL_USER_PROCESS_PARAMETERS process_parameters,
                            PVOID /*PPS_CREATE_INFO*/ create_info,
                            PVOID /*PPS_ATTRIBUTE_LIST*/ attribute_list)
{
  DetourRefCounter ref_count{GetNtCreateUserProcessRefCount()};

  hadesmem::detail::LastErrorPreserver last_error_preserver;
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%p] [%lu] [%lu] [%p] [%p] [%lu] [%lu] [%p] [%p] [%p].",
    process_handle,
    thread_handle,
    process_desired_access,
    thread_desired_access,
    process_object_attributes,
    thread_object_attributes,
    process_flags,
    thread_flags,
    process_parameters,
    create_info,
    attribute_list);
  auto& detour = GetNtCreateUserProcessDetour();
  auto const nt_create_user_process =
    detour->GetTrampoline<decltype(&NtCreateUserProcessDetour)>();
  last_error_preserver.Revert();
  auto const ret = nt_create_user_process(process_handle,
                                          thread_handle,
                                          process_desired_access,
                                          thread_desired_access,
                                          process_object_attributes,
                                          thread_object_attributes,
                                          process_flags,
                                          thread_flags,
                                          process_parameters,
                                          create_info,
                                          attribute_list);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool in_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool in_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  if (in_hook)
  {
    return ret;
  }

  // Need recursion protection because we may need to spawn a process if doing
  // cross-architecture injection.
  hadesmem::detail::RecursionProtector recursion_protector{&in_hook};
  recursion_protector.Set();

  if (!NT_SUCCESS(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
    return ret;
  }

  try
  {
    HADESMEM_DETAIL_ASSERT(process_handle != nullptr);
    DWORD const pid = ::GetProcessId(*process_handle);
    HADESMEM_DETAIL_ASSERT(pid != 0);
    auto const me_wow64 =
      hadesmem::detail::IsWoW64Process(::GetCurrentProcess());
    auto const other_wow64 = hadesmem::detail::IsWoW64Process(*process_handle);
    // Check for architecture mismatch (and use our injector as a 'proxy' in
    // this case).
    // WARNING! In order to locate the correct path to the injector, we assume
    // that the path layout matches that of the build dist output.
    if (me_wow64 != other_wow64)
    {
      auto const self_dir_path = hadesmem::detail::GetSelfDirPath();
      std::wstring const injector_dir = hadesmem::detail::CombinePath(
        self_dir_path, other_wow64 ? L"..\\x86" : L"..\\x64");
      auto const self_path = hadesmem::detail::GetSelfPath();
      auto const module_name = self_path.substr(self_path.rfind(L'\\') + 1);
      auto const command_line =
        injector_dir + L"\\inject.exe --pid " + std::to_wstring(pid) +
        L" --inject --export Load --add-path --path-resolution --module " +
        module_name;
      std::vector<wchar_t> command_line_buf(std::begin(command_line),
                                            std::end(command_line));
      command_line_buf.push_back(L'\0');
      STARTUPINFO start_info;
      ::ZeroMemory(&start_info, sizeof(start_info));
      PROCESS_INFORMATION proc_info;
      ::ZeroMemory(&proc_info, sizeof(proc_info));
      if (!::CreateProcessW(nullptr,
                            command_line_buf.data(),
                            nullptr,
                            nullptr,
                            FALSE,
                            0,
                            nullptr,
                            nullptr,
                            &start_info,
                            &proc_info))
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error{} << hadesmem::ErrorString{"CreateProcess failed."}
                            << hadesmem::ErrorCodeWinLast{last_error});
      }

      hadesmem::detail::SmartHandle const injector_process_handle{
        proc_info.hProcess};
      hadesmem::detail::SmartHandle const injector_thread_handle{
        proc_info.hThread};

      DWORD const wait_res =
        ::WaitForSingleObject(injector_process_handle.GetHandle(), INFINITE);
      if (wait_res != WAIT_OBJECT_0)
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error{}
          << hadesmem::ErrorString{"WaitForSingleObject failed."}
          << hadesmem::ErrorCodeWinLast{last_error});
      }

      DWORD exit_code = 0;
      if (!::GetExitCodeProcess(injector_process_handle.GetHandle(),
                                &exit_code))
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error{}
          << hadesmem::ErrorString{"GetExitCodeProcess failed."}
          << hadesmem::ErrorCodeWinLast{last_error});
      }

      if (exit_code != 0)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error{} << hadesmem::ErrorString{"Injector failed."});
      }
    }
    // Process architectures match, so do it the simple way.
    else
    {
      HADESMEM_DETAIL_ASSERT(pid != 0);
      hadesmem::Process const process{pid};
      auto const module =
        hadesmem::InjectDll(process,
                            hadesmem::detail::GetSelfPath(),
                            hadesmem::InjectFlags::kAddToSearchOrder);
      HADESMEM_DETAIL_TRACE_FORMAT_A("Injected module. [%p]", module);
      auto const export_result = hadesmem::CallExport(process, module, "Load");
      HADESMEM_DETAIL_TRACE_FORMAT_A("Called export. [%Iu] [%lu]",
                                     export_result.GetReturnValue(),
                                     export_result.GetLastError());
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
  hadesmem::Module const ntdll{GetThisProcess(), L"ntdll.dll"};
  auto const nt_query_system_information = hadesmem::FindProcedure(
    GetThisProcess(), ntdll, "NtQuerySystemInformation");
  auto const nt_query_system_information_ptr =
    hadesmem::detail::UnionCast<void*>(nt_query_system_information);
  auto const nt_query_system_information_detour =
    hadesmem::detail::UnionCast<void*>(&NtQuerySystemInformationDetour);
  auto& detour = GetNtQuerySystemInformationDetour();
  detour =
    std::make_unique<hadesmem::PatchDetour>(GetThisProcess(),
                                            nt_query_system_information_ptr,
                                            nt_query_system_information_detour);
  detour->Apply();
  HADESMEM_DETAIL_TRACE_A("NtQuerySystemInformation detoured.");
}

void DetourNtCreateUserProcess()
{
  hadesmem::Module const ntdll{GetThisProcess(), L"ntdll.dll"};
  auto const nt_create_user_process =
    hadesmem::FindProcedure(GetThisProcess(), ntdll, "NtCreateUserProcess");
  auto const nt_create_user_process_ptr =
    hadesmem::detail::UnionCast<void*>(nt_create_user_process);
  auto const nt_create_user_process_detour =
    hadesmem::detail::UnionCast<void*>(&NtCreateUserProcessDetour);
  auto& detour = GetNtCreateUserProcessDetour();
  detour =
    std::make_unique<hadesmem::PatchDetour>(GetThisProcess(),
                                            nt_create_user_process_ptr,
                                            nt_create_user_process_detour);
  detour->Apply();
  HADESMEM_DETAIL_TRACE_A("NtCreateUserProcess detoured.");
}

void UndetourNtQuerySystemInformation()
{
  auto& detour = GetNtQuerySystemInformationDetour();
  detour->Remove();
  HADESMEM_DETAIL_TRACE_A("NtQuerySystemInformation undetoured.");
  detour = nullptr;

  auto& ref_count = GetNtQuerySystemInformationRefCount();
  while (ref_count.load())
  {
    HADESMEM_DETAIL_TRACE_A("Spinning on NtQuerySystemInformation ref count.");
  }
  HADESMEM_DETAIL_TRACE_A("NtQuerySystemInformation free of references.");
}

void UndetourNtCreateUserProcess()
{
  auto& detour = GetNtCreateUserProcessDetour();
  detour->Remove();
  HADESMEM_DETAIL_TRACE_A("NtCreateUserProcess undetoured.");
  detour = nullptr;

  auto& ref_count = GetNtCreateUserProcessRefCount();
  while (ref_count.load())
  {
    HADESMEM_DETAIL_TRACE_A("Spinning on NtCreateUserProcess ref count.");
  }
  HADESMEM_DETAIL_TRACE_A("NtCreateUserProcess free of references.");
}
