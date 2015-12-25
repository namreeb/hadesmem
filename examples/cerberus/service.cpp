// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "service.hpp"

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
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnCreateServiceACallback>&
  GetOnCreateServiceACallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnCreateServiceACallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnCreateServiceWCallback>&
  GetOnCreateServiceWCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnCreateServiceWCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnOpenServiceACallback>&
  GetOnOpenServiceACallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnOpenServiceACallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnOpenServiceWCallback>&
  GetOnOpenServiceWCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnOpenServiceWCallback> callbacks;
  return callbacks;
}

class ServiceImpl : public hadesmem::cerberus::ServiceInterface
{
public:
  virtual std::size_t RegisterOnCreateServiceA(
    std::function<hadesmem::cerberus::OnCreateServiceACallback> const& callback)
    final
  {
    auto& callbacks = GetOnCreateServiceACallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnCreateServiceA(std::size_t id) final
  {
    auto& callbacks = GetOnCreateServiceACallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnCreateServiceW(
    std::function<hadesmem::cerberus::OnCreateServiceWCallback> const& callback)
    final
  {
    auto& callbacks = GetOnCreateServiceWCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnCreateServiceW(std::size_t id) final
  {
    auto& callbacks = GetOnCreateServiceWCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnOpenServiceA(
    std::function<hadesmem::cerberus::OnOpenServiceACallback> const& callback)
    final
  {
    auto& callbacks = GetOnOpenServiceACallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnOpenServiceA(std::size_t id) final
  {
    auto& callbacks = GetOnOpenServiceACallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnOpenServiceW(
    std::function<hadesmem::cerberus::OnOpenServiceWCallback> const& callback)
    final
  {
    auto& callbacks = GetOnOpenServiceWCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnOpenServiceW(std::size_t id) final
  {
    auto& callbacks = GetOnOpenServiceWCallbacks();
    return callbacks.Unregister(id);
  }
};

std::pair<void*, SIZE_T>& GetSechostModule() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::CreateServiceA)>>&
  GetCreateServiceADetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::CreateServiceA)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::CreateServiceW)>>&
  GetCreateServiceWDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::CreateServiceW)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::OpenServiceA)>>&
  GetOpenServiceADetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::OpenServiceA)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::OpenServiceW)>>&
  GetOpenServiceWDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::OpenServiceW)>>
    detour;
  return detour;
}

extern "C" SC_HANDLE WINAPI
  CreateServiceADetour(hadesmem::PatchDetourBase* detour,
                       SC_HANDLE sc_manager,
                       LPCSTR service_name,
                       LPCSTR display_name,
                       DWORD desired_access,
                       DWORD service_type,
                       DWORD start_type,
                       DWORD error_control,
                       LPCSTR binary_path_name,
                       LPCSTR load_order_group,
                       LPDWORD tag_id,
                       LPCSTR dependencies,
                       LPCSTR service_start_name,
                       LPCSTR password) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%p] [%p] [%lu] [%lu] [%lu] "
                                       "[%lu] [%p] [%p] [%p] [%p] [%p] [%p].",
                                       sc_manager,
                                       service_name,
                                       display_name,
                                       desired_access,
                                       service_type,
                                       start_type,
                                       error_control,
                                       binary_path_name,
                                       load_order_group,
                                       tag_id,
                                       dependencies,
                                       service_start_name,
                                       password);

  auto const& callbacks = GetOnCreateServiceACallbacks();
  bool handled = false;
  callbacks.Run(sc_manager,
                service_name,
                display_name,
                desired_access,
                service_type,
                start_type,
                error_control,
                binary_path_name,
                load_order_group,
                tag_id,
                dependencies,
                service_start_name,
                password,
                &handled);

  if (handled)
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "CreateServiceA handled. Not calling trampoline.");
    return nullptr;
  }

  auto const create_service =
    detour->GetTrampolineT<decltype(&::CreateServiceA)>();
  last_error_preserver.Revert();
  auto const ret = create_service(sc_manager,
                                  service_name,
                                  display_name,
                                  desired_access,
                                  service_type,
                                  start_type,
                                  error_control,
                                  binary_path_name,
                                  load_order_group,
                                  tag_id,
                                  dependencies,
                                  service_start_name,
                                  password);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
}

extern "C" SC_HANDLE WINAPI
  CreateServiceWDetour(hadesmem::PatchDetourBase* detour,
                       SC_HANDLE sc_manager,
                       LPCWSTR service_name,
                       LPCWSTR display_name,
                       DWORD desired_access,
                       DWORD service_type,
                       DWORD start_type,
                       DWORD error_control,
                       LPCWSTR binary_path_name,
                       LPCWSTR load_order_group,
                       LPDWORD tag_id,
                       LPCWSTR dependencies,
                       LPCWSTR service_start_name,
                       LPCWSTR password) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%p] [%p] [%lu] [%lu] [%lu] "
                                       "[%lu] [%p] [%p] [%p] [%p] [%p] [%p].",
                                       sc_manager,
                                       service_name,
                                       display_name,
                                       desired_access,
                                       service_type,
                                       start_type,
                                       error_control,
                                       binary_path_name,
                                       load_order_group,
                                       tag_id,
                                       dependencies,
                                       service_start_name,
                                       password);

  auto const& callbacks = GetOnCreateServiceWCallbacks();
  bool handled = false;
  callbacks.Run(sc_manager,
                service_name,
                display_name,
                desired_access,
                service_type,
                start_type,
                error_control,
                binary_path_name,
                load_order_group,
                tag_id,
                dependencies,
                service_start_name,
                password,
                &handled);

  if (handled)
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "CreateServiceA handled. Not calling trampoline.");
    return nullptr;
  }

  auto const create_service =
    detour->GetTrampolineT<decltype(&::CreateServiceW)>();
  last_error_preserver.Revert();
  auto const ret = create_service(sc_manager,
                                  service_name,
                                  display_name,
                                  desired_access,
                                  service_type,
                                  start_type,
                                  error_control,
                                  binary_path_name,
                                  load_order_group,
                                  tag_id,
                                  dependencies,
                                  service_start_name,
                                  password);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
}

extern "C" SC_HANDLE WINAPI
  OpenServiceADetour(hadesmem::PatchDetourBase* detour,
                     SC_HANDLE sc_manager,
                     LPCSTR service_name,
                     DWORD desired_access) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%p] [%lu].", sc_manager, service_name, desired_access);

  auto const& callbacks = GetOnOpenServiceACallbacks();
  bool handled = false;
  callbacks.Run(sc_manager, service_name, desired_access, &handled);

  if (handled)
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "OpenServiceW handled. Not calling trampoline.");
    return nullptr;
  }

  auto const open_service = detour->GetTrampolineT<decltype(&::OpenServiceA)>();
  last_error_preserver.Revert();
  auto const ret = open_service(sc_manager, service_name, desired_access);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
}

extern "C" SC_HANDLE WINAPI
  OpenServiceWDetour(hadesmem::PatchDetourBase* detour,
                     SC_HANDLE sc_manager,
                     LPCWSTR service_name,
                     DWORD desired_access) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%p] [%lu].", sc_manager, service_name, desired_access);

  auto const& callbacks = GetOnOpenServiceWCallbacks();
  bool handled = false;
  callbacks.Run(sc_manager, service_name, desired_access, &handled);

  if (handled)
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "OpenServiceW handled. Not calling trampoline.");
    return nullptr;
  }

  auto const open_service = detour->GetTrampolineT<decltype(&::OpenServiceW)>();
  last_error_preserver.Revert();
  auto const ret = open_service(sc_manager, service_name, desired_access);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
}
}

namespace hadesmem
{
namespace cerberus
{
ServiceInterface& GetServiceInterface() noexcept
{
  static ServiceImpl service_impl;
  return service_impl;
}

void InitializeService()
{
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(L"SECHOST",
                                    DetourSechostForService,
                                    UndetourSechostForService,
                                    GetSechostModule);
}

void DetourSechostForService(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetSechostModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"sechost", base, module))
  {
    DetourFunc(process,
               base,
               "CreateServiceA",
               GetCreateServiceADetour(),
               CreateServiceADetour);
    DetourFunc(process,
               base,
               "CreateServiceW",
               GetCreateServiceWDetour(),
               CreateServiceWDetour);
    DetourFunc(process,
               base,
               "OpenServiceA",
               GetOpenServiceADetour(),
               OpenServiceADetour);
    DetourFunc(process,
               base,
               "OpenServiceW",
               GetOpenServiceWDetour(),
               OpenServiceWDetour);
  }
}

void UndetourSechostForService(bool remove)
{
  auto& module = GetSechostModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"sechost", module))
  {
    UndetourFunc(L"CreateServiceA", GetCreateServiceADetour(), remove);
    UndetourFunc(L"CreateServiceW", GetCreateServiceWDetour(), remove);
    UndetourFunc(L"OpenServiceA", GetOpenServiceADetour(), remove);
    UndetourFunc(L"OpenServiceW", GetOpenServiceWDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}
}
}
