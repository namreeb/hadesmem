// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "dxgi.hpp"

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
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>

#include "callbacks.hpp"
#include "dxgi_factory.hpp"
#include "helpers.hpp"
#include "main.hpp"
#include "module.hpp"

namespace
{
class DXGIImpl : public hadesmem::cerberus::DXGIInterface
{
public:
  virtual std::size_t RegisterOnFrame(
    std::function<hadesmem::cerberus::OnFrameDXGICallback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnFrameDXGICallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnFrame(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnFrameDXGICallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnRelease(
    std::function<hadesmem::cerberus::OnReleaseDXGICallback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnReleaseDXGICallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnRelease(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnReleaseDXGICallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnResize(
    std::function<hadesmem::cerberus::OnResizeDXGICallback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnResizeDXGICallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnResize(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnResizeDXGICallbacks();
    return callbacks.Unregister(id);
  }
};

std::unique_ptr<hadesmem::PatchDetour<decltype(&::CreateDXGIFactory)>>&
  GetCreateDXGIFactoryDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::CreateDXGIFactory)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::CreateDXGIFactory1)>>&
  GetCreateDXGIFactory1Detour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::CreateDXGIFactory1)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::CreateDXGIFactory2)>>&
  GetCreateDXGIFactory2Detour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::CreateDXGIFactory2)>>
    detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetDXGIModule() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

extern "C" HRESULT WINAPI CreateDXGIFactoryDetour(
  hadesmem::PatchDetourBase* detour, REFIID riid, void** factory) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto const create_dxgi_factory =
    detour->GetTrampolineT<decltype(&CreateDXGIFactory)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory(riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");
    HADESMEM_DETAIL_TRACE_A("Proxying IDXGIFactory.");
    *factory = new hadesmem::cerberus::DXGIFactoryProxy{
      static_cast<IDXGIFactory*>(*factory)};
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

extern "C" HRESULT WINAPI CreateDXGIFactory1Detour(
  hadesmem::PatchDetourBase* detour, REFIID riid, void** factory) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto const create_dxgi_factory_1 =
    detour->GetTrampolineT<decltype(&CreateDXGIFactory1)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory_1(riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");
    HADESMEM_DETAIL_TRACE_A("Proxying IDXGIFactory1.");
    *factory = new hadesmem::cerberus::DXGIFactoryProxy{
      static_cast<IDXGIFactory1*>(*factory)};
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

extern "C" HRESULT WINAPI
  CreateDXGIFactory2Detour(hadesmem::PatchDetourBase* detour,
                           UINT flags,
                           REFIID riid,
                           void** factory) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto const create_dxgi_factory_2 =
    detour->GetTrampolineT<decltype(&CreateDXGIFactory2)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory_2(flags, riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");
    HADESMEM_DETAIL_TRACE_A("Proxying IDXGIFactory2.");
    *factory = new hadesmem::cerberus::DXGIFactoryProxy{
      static_cast<IDXGIFactory*>(*factory)};
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}
}

namespace hadesmem
{
namespace cerberus
{
Callbacks<OnFrameDXGICallback>& GetOnFrameDXGICallbacks()
{
  static Callbacks<OnFrameDXGICallback> callbacks;
  return callbacks;
}

Callbacks<OnReleaseDXGICallback>& GetOnReleaseDXGICallbacks()
{
  static Callbacks<OnReleaseDXGICallback> callbacks;
  return callbacks;
}

Callbacks<OnResizeDXGICallback>& GetOnResizeDXGICallbacks()
{
  static Callbacks<OnResizeDXGICallback> callbacks;
  return callbacks;
}

DXGIInterface& GetDXGIInterface() noexcept
{
  static DXGIImpl dxgi_impl;
  return dxgi_impl;
}

void InitializeDXGI()
{
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(
    L"DXGI", &DetourDXGI, &UndetourDXGI, &GetDXGIModule);
}

void DetourDXGI(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetDXGIModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"DXGI", base, module))
  {
    DetourFunc(process,
               base,
               "CreateDXGIFactory",
               GetCreateDXGIFactoryDetour(),
               CreateDXGIFactoryDetour);
    DetourFunc(process,
               base,
               "CreateDXGIFactory1",
               GetCreateDXGIFactory1Detour(),
               CreateDXGIFactory1Detour);
    DetourFunc(process,
               base,
               "CreateDXGIFactory2",
               GetCreateDXGIFactory2Detour(),
               CreateDXGIFactory2Detour);
  }
}

void UndetourDXGI(bool remove)
{
  auto& module = GetDXGIModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"DXGI", module))
  {
    UndetourFunc(L"CreateDXGIFactory", GetCreateDXGIFactoryDetour(), remove);
    UndetourFunc(L"CreateDXGIFactory1", GetCreateDXGIFactory1Detour(), remove);
    UndetourFunc(L"CreateDXGIFactory2", GetCreateDXGIFactory2Detour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}
}
}
