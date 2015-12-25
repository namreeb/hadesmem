// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "driver.hpp"

#include <memory>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "callbacks.hpp"
#include "main.hpp"
#include "helpers.hpp"

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnNtLoadDriverCallback>&
  GetOnNtLoadDriverCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnNtLoadDriverCallback> callbacks;
  return callbacks;
}

class DriverImpl : public hadesmem::cerberus::DriverInterface
{
public:
  virtual std::size_t RegisterOnNtLoadDriver(
    std::function<hadesmem::cerberus::OnNtLoadDriverCallback> const& callback)
    final
  {
    auto& callbacks = GetOnNtLoadDriverCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnNtLoadDriver(std::size_t id) final
  {
    auto& callbacks = GetOnNtLoadDriverCallbacks();
    return callbacks.Unregister(id);
  }
};

std::pair<void*, SIZE_T>& GetNtdllModule() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

typedef NTSTATUS(NTAPI* NtLoadDriverFn)(PUNICODE_STRING driver_service_name);

std::unique_ptr<hadesmem::PatchDetour<NtLoadDriverFn>>&
  GetNtLoadDriverDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<NtLoadDriverFn>> detour;
  return detour;
}

extern "C" NTSTATUS NTAPI
  NtLoadDriverDetour(hadesmem::PatchDetourBase* detour,
                     PUNICODE_STRING driver_service_name) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", driver_service_name);

  if (driver_service_name)
  {
    HADESMEM_DETAIL_TRACE_NOISY_FORMAT_W(
      L"Name: [%s].",
      hadesmem::detail::UnicodeStringToStdString(driver_service_name).c_str());
  }

  auto const& callbacks = GetOnNtLoadDriverCallbacks();
  bool handled = false;
  callbacks.Run(driver_service_name, &handled);

  if (handled)
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "NtLoadDriver handled. Not calling trampoline.");
    return STATUS_INVALID_PARAMETER;
  }

  auto const nt_load_driver = detour->GetTrampolineT<NtLoadDriverFn>();
  last_error_preserver.Revert();
  auto const ret = nt_load_driver(driver_service_name);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
}
}

namespace hadesmem
{
namespace cerberus
{
DriverInterface& GetDriverInterface() noexcept
{
  static DriverImpl driver_impl;
  return driver_impl;
}

void InitializeDriver()
{
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(
    L"NTDLL", DetourNtdllForDriver, UndetourNtdllForDriver, GetNtdllModule);
}

void DetourNtdllForDriver(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetNtdllModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"ntdll", base, module))
  {
    DetourFunc(process,
               base,
               "NtLoadDriver",
               GetNtLoadDriverDetour(),
               NtLoadDriverDetour);
  }
}

void UndetourNtdllForDriver(bool remove)
{
  auto& module = GetNtdllModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"ntdll", module))
  {
    UndetourFunc(L"NtLoadDriver", GetNtLoadDriverDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}
}
}
