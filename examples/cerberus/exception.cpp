// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "exception.hpp"

#include <atomic>
#include <memory>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "detour_ref_counter.hpp"
#include "main.hpp"

namespace
{

std::unique_ptr<hadesmem::PatchDetour>&
  GetRtlAddVectoredExceptionHandlerDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetRtlAddVectoredExceptionHandlerRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

extern "C" PVOID WINAPI RtlAddVectoredExceptionHandlerDetour(
  ULONG first_handler, PVECTORED_EXCEPTION_HANDLER vectored_handler)
  HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::cerberus::DetourRefCounter ref_count{
    GetRtlAddVectoredExceptionHandlerRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%lu] [%p].", first_handler, vectored_handler);
  auto& detour = GetRtlAddVectoredExceptionHandlerDetour();
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
  Module const ntdll{GetThisProcess(), L"ntdll.dll"};
  auto const rtl_add_vectored_exception_handler =
    FindProcedure(GetThisProcess(), ntdll, "RtlAddVectoredExceptionHandler");
  auto const rtl_add_vectored_exception_handler_ptr =
    detail::UnionCast<void*>(rtl_add_vectored_exception_handler);
  auto const rtl_add_vectored_exception_handler_detour =
    detail::UnionCast<void*>(&RtlAddVectoredExceptionHandlerDetour);
  auto& detour = GetRtlAddVectoredExceptionHandlerDetour();
  detour =
    std::make_unique<PatchDetour>(GetThisProcess(),
                                  rtl_add_vectored_exception_handler_ptr,
                                  rtl_add_vectored_exception_handler_detour);
  detour->Apply();
  HADESMEM_DETAIL_TRACE_A("RtlAddVectoredExceptionHandler detoured.");
}

void UndetourRtlAddVectoredExceptionHandler()
{
  auto& detour = GetRtlAddVectoredExceptionHandlerDetour();
  detour->Remove();
  HADESMEM_DETAIL_TRACE_A("RtlAddVectoredExceptionHandler undetoured.");
  detour = nullptr;

  auto& ref_count = GetRtlAddVectoredExceptionHandlerRefCount();
  while (ref_count.load())
  {
    HADESMEM_DETAIL_TRACE_A(
      "Spinning on RtlAddVectoredExceptionHandler ref count.");
  }
  HADESMEM_DETAIL_TRACE_A("RtlAddVectoredExceptionHandler free of references.");
}
}
}
