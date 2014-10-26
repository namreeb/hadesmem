// Copyright (C) 2010-2014 Joshua Boyce.
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

std::unique_ptr<hadesmem::PatchDetour>&
  GetRtlAddVectoredExceptionHandlerDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

extern "C" PVOID WINAPI RtlAddVectoredExceptionHandlerDetour(
  ULONG first_handler,
  PVECTORED_EXCEPTION_HANDLER vectored_handler) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetRtlAddVectoredExceptionHandlerDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%lu] [%p].", first_handler, vectored_handler);
  auto const rtl_add_vectored_exception_handler =
    detour->GetTrampoline<decltype(&RtlAddVectoredExceptionHandlerDetour)>();
  last_error_preserver.Revert();
  auto const ret =
    rtl_add_vectored_exception_handler(first_handler, vectored_handler);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}
}

namespace hadesmem
{

namespace cerberus
{

void DetourRtlAddVectoredExceptionHandler()
{
  auto const& process = GetThisProcess();
  auto const kernelbase_mod = ::GetModuleHandleW(L"ntdll");
  DetourFunc(process,
             kernelbase_mod,
             "RtlAddVectoredExceptionHandler",
             GetRtlAddVectoredExceptionHandlerDetour(),
             RtlAddVectoredExceptionHandlerDetour);
}

void UndetourRtlAddVectoredExceptionHandler()
{
  UndetourFunc(L"RtlAddVectoredExceptionHandler",
               GetRtlAddVectoredExceptionHandlerDetour(),
               true);
}
}
}
