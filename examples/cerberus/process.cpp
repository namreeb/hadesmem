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
#include <hadesmem/detail/detour_ref_counter.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/recursion_protector.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"

namespace
{

std::unique_ptr<hadesmem::PatchDetour>& GetCreateProcessInternalWDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetCreateProcessInternalWRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

extern "C" BOOL WINAPI
  CreateProcessInternalWDetour(HANDLE token,
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
                               PHANDLE new_token)
{
  hadesmem::detail::DetourRefCounter ref_count{
    GetCreateProcessInternalWRefCount()};
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
  auto& detour = GetCreateProcessInternalWDetour();
  auto const nt_create_user_process =
    detour->GetTrampoline<decltype(&CreateProcessInternalWDetour)>();
  last_error_preserver.Revert();
  auto const ret = nt_create_user_process(token,
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

  // Need recursion protection because we may spawn a new process as a proxy for
  // cross-architecture injection.
  hadesmem::detail::RecursionProtector recursion_protector{&in_hook};
  recursion_protector.Set();

  if (!ret)
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
    return ret;
  }

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

namespace hadesmem
{

namespace cerberus
{

void DetourCreateProcessInternalW()
{
  Module const kernel32{GetThisProcess(), L"kernelbase.dll"};
  auto const create_process_internal_w =
    FindProcedure(GetThisProcess(), kernel32, "CreateProcessInternalW");
  auto const create_process_internal_w_ptr =
    detail::UnionCast<void*>(create_process_internal_w);
  auto const create_process_internal_w_detour =
    detail::UnionCast<void*>(&CreateProcessInternalWDetour);
  auto& detour = GetCreateProcessInternalWDetour();
  detour = std::make_unique<PatchDetour>(GetThisProcess(),
                                         create_process_internal_w_ptr,
                                         create_process_internal_w_detour);
  detour->Apply();
  HADESMEM_DETAIL_TRACE_A("CreateProcessInternalW detoured.");
}

void UndetourCreateProcessInternalW()
{
  auto& detour = GetCreateProcessInternalWDetour();
  detour->Remove();
  HADESMEM_DETAIL_TRACE_A("CreateProcessInternalW undetoured.");
  detour = nullptr;

  auto& ref_count = GetCreateProcessInternalWRefCount();
  while (ref_count.load())
  {
    HADESMEM_DETAIL_TRACE_A("Spinning on CreateProcessInternalW ref count.");
  }
  HADESMEM_DETAIL_TRACE_A("CreateProcessInternalW free of references.");
}
}
}
