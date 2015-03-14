// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "exception.hpp"

#include <memory>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/detour_ref_counter.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"
#include "helpers.hpp"

namespace
{
std::pair<void*, SIZE_T>& GetNtdllModule() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

std::pair<void*, SIZE_T>& GetKernelBaseModule() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

std::unique_ptr<
  hadesmem::PatchDetour<decltype(&::AddVectoredExceptionHandler)>>&
  GetRtlAddVectoredExceptionHandlerDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<
    hadesmem::PatchDetour<decltype(&::AddVectoredExceptionHandler)>> detour;
  return detour;
}

std::unique_ptr<
  hadesmem::PatchDetour<decltype(&::SetUnhandledExceptionFilter)>>&
  GetSetUnhandledExceptionFilterDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<
    hadesmem::PatchDetour<decltype(&::SetUnhandledExceptionFilter)>> detour;
  return detour;
}

extern "C" PVOID WINAPI RtlAddVectoredExceptionHandlerDetour(
  hadesmem::PatchDetourBase* detour,
  ULONG first_handler,
  PVECTORED_EXCEPTION_HANDLER vectored_handler) HADESMEM_DETAIL_NOEXCEPT
{
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%lu] [%p].", first_handler, vectored_handler);
  auto const rtl_add_vectored_exception_handler =
    detour->GetTrampolineT<decltype(&AddVectoredExceptionHandler)>();
  last_error_preserver.Revert();
  auto const ret =
    rtl_add_vectored_exception_handler(first_handler, vectored_handler);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%p].", ret);

  return ret;
}

extern "C" LPTOP_LEVEL_EXCEPTION_FILTER WINAPI
  SetUnhandledExceptionFilterDetour(
    hadesmem::PatchDetourBase* detour,
    LPTOP_LEVEL_EXCEPTION_FILTER top_level_exception_filter)
    HADESMEM_DETAIL_NOEXCEPT
{
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p].", top_level_exception_filter);
  auto const set_unhandled_exception_filter =
    detour->GetTrampolineT<decltype(&SetUnhandledExceptionFilter)>();
  last_error_preserver.Revert();
  auto const ret = set_unhandled_exception_filter(top_level_exception_filter);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%p].", ret);

  return ret;
}
}

namespace hadesmem
{
namespace cerberus
{
void InitializeException()
{
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(L"NTDLL",
                                    DetourNtdllForException,
                                    UndetourNtdllForException,
                                    GetNtdllModule);
  helper.InitializeSupportForModule(L"KERNELBASE",
                                    DetourKernelBaseForException,
                                    UndetourNtdllForException,
                                    GetKernelBaseModule);
}

void DetourNtdllForException(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetNtdllModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"ntdll", base, module))
  {
    DetourFunc(process,
               base,
               "RtlAddVectoredExceptionHandler",
               GetRtlAddVectoredExceptionHandlerDetour(),
               RtlAddVectoredExceptionHandlerDetour);
  }
}

void DetourKernelBaseForException(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetKernelBaseModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"kernelbase", base, module))
  {
    DetourFunc(process,
               base,
               "SetUnhandledExceptionFilter",
               GetSetUnhandledExceptionFilterDetour(),
               SetUnhandledExceptionFilterDetour);
  }
}

void UndetourNtdllForException(bool remove)
{
  auto& module = GetNtdllModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"ntdll", module))
  {
    UndetourFunc(L"RtlAddVectoredExceptionHandler",
                 GetRtlAddVectoredExceptionHandlerDetour(),
                 remove);

    module = std::make_pair(nullptr, 0);
  }
}

void UndetourKernelBaseForException(bool remove)
{
  auto& module = GetKernelBaseModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"kernelbase", module))
  {
    UndetourFunc(L"SetUnhandledExceptionFilter",
                 GetSetUnhandledExceptionFilterDetour(),
                 remove);

    module = std::make_pair(nullptr, 0);
  }
}
}
}
