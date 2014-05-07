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
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/injector.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "detour_ref_counter.hpp"
#include "main.hpp"

namespace
{

std::unique_ptr<hadesmem::PatchDetour>& GetNtCreateUserProcessDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetNtCreateUserProcessRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
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
  HADESMEM_DETAIL_NOEXCEPT
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
    if (me_wow64 == other_wow64)
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
    else
    {
      HADESMEM_DETAIL_TRACE_A("Warning! Cross-architecture injection is "
                              "currently unsupported. Ignoring new process.");
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
