// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "exception.hpp"

#include <memory>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "callbacks.hpp"
#include "main.hpp"
#include "helpers.hpp"

// TODO: Add built-in exception handler registration blocking via a default-off
// config flag.

namespace
{
hadesmem::cerberus::Callbacks<
  hadesmem::cerberus::OnRtlAddVectoredExceptionHandlerCallback>&
  GetOnRtlAddVectoredExceptionHandlerCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnRtlAddVectoredExceptionHandlerCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<
  hadesmem::cerberus::OnSetUnhandledExceptionFilterCallback>&
  GetOnSetUnhandledExceptionFilterCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnSetUnhandledExceptionFilterCallback> callbacks;
  return callbacks;
}

class ExceptionImpl : public hadesmem::cerberus::ExceptionInterface
{
public:
  virtual std::size_t RegisterOnRtlAddVectoredExceptionHandler(
    std::function<
      hadesmem::cerberus::OnRtlAddVectoredExceptionHandlerCallback> const&
      callback) final
  {
    auto& callbacks = GetOnRtlAddVectoredExceptionHandlerCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnRtlAddVectoredExceptionHandler(std::size_t id) final
  {
    auto& callbacks = GetOnRtlAddVectoredExceptionHandlerCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnSetUnhandledExceptionFilter(
    std::function<
      hadesmem::cerberus::OnSetUnhandledExceptionFilterCallback> const&
      callback) final
  {
    auto& callbacks = GetOnSetUnhandledExceptionFilterCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnSetUnhandledExceptionFilter(std::size_t id) final
  {
    auto& callbacks = GetOnSetUnhandledExceptionFilterCallbacks();
    return callbacks.Unregister(id);
  }
};

std::pair<void*, SIZE_T>& GetNtdllModule() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

std::pair<void*, SIZE_T>& GetKernelBaseModule() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

std::unique_ptr<
  hadesmem::PatchDetour<decltype(&::AddVectoredExceptionHandler)>>&
  GetRtlAddVectoredExceptionHandlerDetour() noexcept
{
  static std::unique_ptr<
    hadesmem::PatchDetour<decltype(&::AddVectoredExceptionHandler)>> detour;
  return detour;
}

std::unique_ptr<
  hadesmem::PatchDetour<decltype(&::SetUnhandledExceptionFilter)>>&
  GetSetUnhandledExceptionFilterDetour() noexcept
{
  static std::unique_ptr<
    hadesmem::PatchDetour<decltype(&::SetUnhandledExceptionFilter)>> detour;
  return detour;
}

extern "C" PVOID WINAPI RtlAddVectoredExceptionHandlerDetour(
  hadesmem::PatchDetourBase* detour,
  ULONG first_handler,
  PVECTORED_EXCEPTION_HANDLER vectored_handler) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%lu] [%p].", first_handler, vectored_handler);

  auto const& callbacks = GetOnRtlAddVectoredExceptionHandlerCallbacks();
  bool handled = false;
  callbacks.Run(first_handler, vectored_handler, &handled);

  if (handled)
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "RtlAddVectoredExceptionHandler handled. Not calling trampoline.");
    return nullptr;
  }

  auto const rtl_add_vectored_exception_handler =
    detour->GetTrampolineT<decltype(&AddVectoredExceptionHandler)>();
  last_error_preserver.Revert();
  auto const ret =
    rtl_add_vectored_exception_handler(first_handler, vectored_handler);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
}

extern "C" LPTOP_LEVEL_EXCEPTION_FILTER WINAPI
  SetUnhandledExceptionFilterDetour(
    hadesmem::PatchDetourBase* detour,
    LPTOP_LEVEL_EXCEPTION_FILTER top_level_exception_filter) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p].", top_level_exception_filter);

  auto const& callbacks = GetOnSetUnhandledExceptionFilterCallbacks();
  bool handled = false;
  callbacks.Run(top_level_exception_filter, &handled);

  if (handled)
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "SetUnhandledExceptionFilter handled. Not calling trampoline.");
    return nullptr;
  }

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
ExceptionInterface& GetExceptionInterface() noexcept
{
  static ExceptionImpl exception_impl;
  return exception_impl;
}

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
