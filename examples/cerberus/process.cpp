// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "process.hpp"

#include <algorithm>
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
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "callbacks.hpp"
#include "config.hpp"
#include "helpers.hpp"
#include "main.hpp"

namespace
{
auto& GetOnCreateProcessInternalWCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnCreateProcessInternalWCallback>
    callbacks;
  return callbacks;
}

auto& GetOnRtlExitUserProcessCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnRtlExitUserProcessCallback>
    callbacks;
  return callbacks;
}

class ProcessImpl : public hadesmem::cerberus::ProcessInterface
{
public:
  virtual std::size_t RegisterOnCreateProcessInternalW(
    std::function<hadesmem::cerberus::OnCreateProcessInternalWCallback> const&
      callback) final
  {
    auto& callbacks = GetOnCreateProcessInternalWCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnCreateProcessInternalW(std::size_t id) final
  {
    auto& callbacks = GetOnCreateProcessInternalWCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnRtlExitUserProcess(
    std::function<hadesmem::cerberus::OnRtlExitUserProcessCallback> const&
      callback) final
  {
    auto& callbacks = GetOnRtlExitUserProcessCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnRtlExitUserProcess(std::size_t id) final
  {
    auto& callbacks = GetOnRtlExitUserProcessCallbacks();
    return callbacks.Unregister(id);
  }
};

std::pair<void*, SIZE_T>& GetKernelBaseModule() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

std::pair<void*, SIZE_T>& GetNtdllModule() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

class EnsureResumeThread
{
public:
  explicit EnsureResumeThread(HANDLE handle) noexcept : handle_(handle)
  {
  }

  ~EnsureResumeThread()
  {
    try
    {
      Cleanup();
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);
    }
  }

private:
  void Cleanup()
  {
    if (ResumeThread(handle_) == static_cast<DWORD>(-1))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"ResumeThread failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
  }

  HANDLE handle_;
};

extern "C" BOOL WINAPI
  CreateProcessInternalW(HANDLE token,
                         LPCWSTR application_name,
                         LPWSTR command_line,
                         LPSECURITY_ATTRIBUTES process_attributes,
                         LPSECURITY_ATTRIBUTES thread_attributes,
                         BOOL inherit_handles,
                         DWORD creation_flags,
                         LPVOID environment,
                         LPCWSTR current_directory,
                         LPSTARTUPINFOW startup_info,
                         LPPROCESS_INFORMATION process_info,
                         PHANDLE new_token);

std::unique_ptr<hadesmem::PatchDetour<decltype(&CreateProcessInternalW)>>&
  GetCreateProcessInternalWDetour() noexcept
{
  static std::unique_ptr<
    hadesmem::PatchDetour<decltype(&CreateProcessInternalW)>>
    detour;
  return detour;
}

extern "C" BOOL WINAPI
  CreateProcessInternalWDetour(hadesmem::PatchDetourBase* detour,
                               HANDLE token,
                               LPCWSTR application_name,
                               LPWSTR command_line,
                               LPSECURITY_ATTRIBUTES process_attributes,
                               LPSECURITY_ATTRIBUTES thread_attributes,
                               BOOL inherit_handles,
                               DWORD creation_flags,
                               LPVOID environment,
                               LPCWSTR current_directory,
                               LPSTARTUPINFOW startup_info,
                               LPPROCESS_INFORMATION process_info,
                               PHANDLE new_token) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%p] [%p] [%p] [%p] [%d] [%lu] [%p] [%p] [%p] [%p] [%p].",
    token,
    application_name,
    command_line,
    process_attributes,
    thread_attributes,
    inherit_handles,
    creation_flags,
    environment,
    current_directory,
    startup_info,
    process_info,
    new_token);

  if (application_name)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Application Name: [%s]", application_name);
  }

  if (command_line)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Command Line: [%s]", command_line);
  }

  if (!!(creation_flags & DEBUG_PROCESS))
  {
    HADESMEM_DETAIL_TRACE_A("Debug flag detected.");
  }

  auto const& callbacks = GetOnCreateProcessInternalWCallbacks();
  bool handled = false;
  BOOL retval = FALSE;
  bool suspend = false;
  callbacks.Run(token,
                application_name,
                command_line,
                process_attributes,
                thread_attributes,
                inherit_handles,
                creation_flags,
                environment,
                current_directory,
                startup_info,
                process_info,
                new_token,
                &handled,
                &retval,
                &suspend);

  if (handled)
  {
    HADESMEM_DETAIL_TRACE_A(
      "CreateProcessInternalW handled. Not calling trampoline.");
    return retval;
  }

  // TODO: Add support for this in the config, similar to blocked processes.
  if (suspend)
  {
    HADESMEM_DETAIL_TRACE_A("Process will be created suspended.");
  }

  std::wstring const application_name_str(
    application_name ? hadesmem::detail::ToUpperOrdinal(application_name)
                     : L"");
  std::wstring const command_line_str(
    command_line ? hadesmem::detail::ToUpperOrdinal(command_line) : L"");

  auto const& config = hadesmem::cerberus::GetConfig();
  auto const blocked_processes = config.GetBlockedProcesses();
  for (auto const& blocked_name : blocked_processes)
  {
    auto const blocked_name_upper =
      hadesmem::detail::ToUpperOrdinal(blocked_name);

    // TODO: Actually parse the command line and implement this properly.
    if (application_name_str.find(blocked_name_upper) != std::wstring::npos ||
        command_line_str.find(blocked_name_upper) != std::wstring::npos)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Blocking launch of process. Name: [%s].",
                                     blocked_name.c_str());
      ::SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }
  }

  // TODO: Add a 'skip list' that we don't inject into. Things like error
  // reporting processes, overlay hosts, etc. that are just a source of noise
  // and have no value being hooked.

  auto const create_process_internal_w =
    detour->GetTrampolineT<decltype(&CreateProcessInternalW)>();
  last_error_preserver.Revert();
  auto const ret = create_process_internal_w(token,
                                             application_name,
                                             command_line,
                                             process_attributes,
                                             thread_attributes,
                                             inherit_handles,
                                             creation_flags | CREATE_SUSPENDED,
                                             environment,
                                             current_directory,
                                             startup_info,
                                             process_info,
                                             new_token);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  std::unique_ptr<EnsureResumeThread> resume_thread;
  if (ret && !(creation_flags & CREATE_SUSPENDED) && !suspend)
  {
    resume_thread.reset(new EnsureResumeThread(process_info->hThread));
  }

  if (hadesmem::cerberus::GetDisableCreateProcessInternalWHook())
  {
    HADESMEM_DETAIL_TRACE_A("Hook disabled.");
    return ret;
  }

  thread_local static std::int32_t in_hook = 0;
  if (in_hook)
  {
    HADESMEM_DETAIL_TRACE_A("Recursion detected.");
    return ret;
  }

  // Need recursion protection because we may spawn a new process as a proxy for
  // cross-architecture injection.
  hadesmem::detail::RecursionProtector recursion_protector{&in_hook};

  if (!ret)
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
    return ret;
  }

  // TODO: Add an option to use SetWindowsHookEx based injection for games which
  // utilize EAC and similar anti-cheats (does it work vs XignCode?). We should
  // be okay to do this because OBS needs to do the same thing.

  try
  {
    HADESMEM_DETAIL_ASSERT(process_info != nullptr);
    DWORD const pid = process_info->dwProcessId;
    HADESMEM_DETAIL_ASSERT(pid != 0);
    auto const me_wow64 =
      hadesmem::detail::IsWoW64Process(::GetCurrentProcess());
    HANDLE const process_handle = process_info->hProcess;
    HADESMEM_DETAIL_ASSERT(process_handle != nullptr);
    auto const other_wow64 = hadesmem::detail::IsWoW64Process(process_handle);
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
      auto const injector_command_line =
        L"\"" + injector_dir + L"\\inject.exe\" --pid " + std::to_wstring(pid) +
        L" --inject --export Load --add-path --path-resolution --module " +
        module_name;
      std::vector<wchar_t> command_line_buf(std::begin(injector_command_line),
                                            std::end(injector_command_line));
      command_line_buf.push_back(L'\0');
      STARTUPINFO start_info{};
      PROCESS_INFORMATION proc_info{};
      // TODO: Capture stdout and debug trace it.
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
          hadesmem::Error{} << hadesmem::ErrorString{"CreateProcessW failed."}
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
      hadesmem::Process const process{pid};
      auto const module =
        hadesmem::InjectDll(process,
                            hadesmem::detail::GetSelfPath(),
                            hadesmem::InjectFlags::kAddToSearchOrder);
      HADESMEM_DETAIL_TRACE_FORMAT_A("Injected module. [%p]", module);
      auto const export_result = hadesmem::CallExport(process, module, "Load");
      (void)export_result;
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

extern "C" void WINAPI RtlExitUserProcess(NTSTATUS exit_status);

auto& GetRtlExitUserProcessDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&RtlExitUserProcess)>>
    detour;
  return detour;
}

// TODO: Remove the need for this. It's awful and causes just as many problems
// as it solves.
extern "C" void WINAPI RtlExitUserProcessDetour(
  hadesmem::PatchDetourBase* detour, NTSTATUS exit_status) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%ld].", exit_status);

  auto const& callbacks = GetOnRtlExitUserProcessCallbacks();
  callbacks.Run(exit_status);

  auto const rtl_exit_user_process =
    detour->GetTrampolineT<decltype(&RtlExitUserProcess)>();
  last_error_preserver.Revert();
  rtl_exit_user_process(exit_status);
  last_error_preserver.Update();
}
}

namespace hadesmem
{
namespace cerberus
{
ProcessInterface& GetProcessInterface() noexcept
{
  static ProcessImpl exception_impl;
  return exception_impl;
}

void InitializeProcess()
{
  // TODO: Hook CreateProcessInternalW in Kernel32 on W7 because it's not in
  // KernelBase.
  // TODO: Investigate hooking the NTDLL process creation APIs instead. May
  // cause issues due to process not being a Win32 process at time we attempt to
  // inject.
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(L"KERNELBASE",
                                    DetourKernelBaseForProcess,
                                    UndetourKernelBaseForProcess,
                                    GetKernelBaseModule);
  helper.InitializeSupportForModule(
    L"NTDLL", DetourNtdllForProcess, UndetourNtdllForProcess, GetNtdllModule);
}

void DetourKernelBaseForProcess(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetKernelBaseModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"kernelbase", base, module))
  {
    DetourFunc(process,
               base,
               "CreateProcessInternalW",
               GetCreateProcessInternalWDetour(),
               CreateProcessInternalWDetour);
  }
}

void UndetourKernelBaseForProcess(bool remove)
{
  auto& module = GetKernelBaseModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"kernelbase", module))
  {
    UndetourFunc(
      L"CreateProcessInternalW", GetCreateProcessInternalWDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}

void DetourNtdllForProcess(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetNtdllModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"ntdll", base, module))
  {
    DetourFunc(process,
               base,
               "RtlExitUserProcess",
               GetRtlExitUserProcessDetour(),
               RtlExitUserProcessDetour);
  }
}

void UndetourNtdllForProcess(bool remove)
{
  auto& module = GetNtdllModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"ntdll", module))
  {
    UndetourFunc(L"RtlExitUserProcess", GetRtlExitUserProcessDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}

bool& GetDisableCreateProcessInternalWHook() noexcept
{
  thread_local static bool disable_hook = false;
  return disable_hook;
}
}
}
