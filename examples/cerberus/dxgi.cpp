// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "dxgi.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <dxgi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/detour_ref_counter.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>

#include "callbacks.hpp"
#include "dxgi_helpers.hpp"
#include "helpers.hpp"
#include "main.hpp"
#include "module.hpp"

namespace
{

class DXGIImpl : public hadesmem::cerberus::DXGIInterface
{
public:
  virtual std::size_t RegisterOnFrameCallback(std::function<
    hadesmem::cerberus::OnFrameCallbackDXGI> const& callback) final
  {
    return hadesmem::cerberus::RegisterOnFrameCallbackDXGI(callback);
  }

  virtual void UnregisterOnFrameCallback(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnFrameCallbackDXGI(id);
  }
};

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDXGISwapChainPresentDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDXGIFactoryCreateSwapChainDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetCreateDXGIFactoryDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetCreateDXGIFactory1Detour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetCreateDXGIFactory2Detour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetDXGIModule() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackDXGI>&
  GetOnFrameCallbacksDXGI()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackDXGI>
    callbacks;
  return callbacks;
}

extern "C" HRESULT WINAPI
  IDXGISwapChainPresentDetour(IDXGISwapChain* swap_chain,
                              UINT sync_interval,
                              UINT flags) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGISwapChainPresentDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%u] [%u].", swap_chain, sync_interval, flags);

  auto& callbacks = GetOnFrameCallbacksDXGI();
  callbacks.Run(swap_chain);

  auto const present =
    detour->GetTrampoline<decltype(&IDXGISwapChainPresentDetour)>();
  last_error_preserver.Revert();
  auto const ret = present(swap_chain, sync_interval, flags);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);
  return ret;
}

extern "C" HRESULT WINAPI IDXGIFactoryCreateSwapChainDetour(
  IDXGIFactory* factory,
  IUnknown* device,
  DXGI_SWAP_CHAIN_DESC* desc,
  IDXGISwapChain** swap_chain) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDXGIFactoryCreateSwapChainDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%p] [%p] [%p].", factory, device, desc, swap_chain);
  auto const create_swap_chain =
    detour->GetTrampoline<decltype(&IDXGIFactoryCreateSwapChainDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_swap_chain(factory, device, desc, swap_chain);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (swap_chain)
    {
      hadesmem::cerberus::DetourDXGISwapChain(*swap_chain);
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid swap chain out param pointer.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

void DetourDXGIFactoryByGuid(REFIID riid, void** factory)
{

  if (riid == __uuidof(IDXGIFactory))
  {
    hadesmem::cerberus::DetourDXGIFactory(static_cast<IDXGIFactory*>(*factory));
  }
  else if (riid == __uuidof(IDXGIFactory1))
  {
    hadesmem::cerberus::DetourDXGIFactory(
      static_cast<IDXGIFactory1*>(*factory));
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Unsupported GUID.");
  }
}

extern "C" HRESULT WINAPI
  CreateDXGIFactoryDetour(REFIID riid, void** factory) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetCreateDXGIFactoryDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto const create_dxgi_factory =
    detour->GetTrampoline<decltype(&CreateDXGIFactoryDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory(riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    DetourDXGIFactoryByGuid(riid, factory);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

extern "C" HRESULT WINAPI
  CreateDXGIFactory1Detour(REFIID riid, void** factory) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetCreateDXGIFactory1Detour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto const create_dxgi_factory_1 =
    detour->GetTrampoline<decltype(&CreateDXGIFactory1Detour)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory_1(riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    DetourDXGIFactoryByGuid(riid, factory);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

extern "C" HRESULT WINAPI
  CreateDXGIFactory2Detour(UINT flags,
                           REFIID riid,
                           void** factory) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetCreateDXGIFactory2Detour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto const create_dxgi_factory_2 =
    detour->GetTrampoline<decltype(&CreateDXGIFactory2Detour)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory_2(flags, riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    DetourDXGIFactoryByGuid(riid, factory);
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

DXGIInterface& GetDXGIInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static DXGIImpl dxgi_impl;
  return dxgi_impl;
}

void InitializeDXGI()
{
  InitializeSupportForModule(
    L"DXGI", &DetourDXGI, &UndetourDXGI, &GetDXGIModule);
}

void DetourDXGI(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetDXGIModule();
  if (CommonDetourModule(process, L"DXGI", base, module))
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
  if (CommonUndetourModule(L"DXGI", module))
  {
    UndetourFunc(L"CreateDXGIFactory", GetCreateDXGIFactoryDetour(), remove);
    UndetourFunc(L"CreateDXGIFactory1", GetCreateDXGIFactory1Detour(), remove);
    UndetourFunc(L"CreateDXGIFactory2", GetCreateDXGIFactory2Detour(), remove);
    UndetourFunc(L"IDXGIFactory::CreateSwapChain",
                 GetIDXGIFactoryCreateSwapChainDetour(),
                 remove);
    UndetourFunc(
      L"IDXGISwapChain::Present", GetIDXGISwapChainPresentDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}

std::size_t RegisterOnFrameCallbackDXGI(
  std::function<OnFrameCallbackDXGI> const& callback)
{
  auto& callbacks = GetOnFrameCallbacksDXGI();
  return callbacks.Register(callback);
}

void UnregisterOnFrameCallbackDXGI(std::size_t id)
{
  auto& callbacks = GetOnFrameCallbacksDXGI();
  return callbacks.Unregister(id);
}

void DetourDXGISwapChain(IDXGISwapChain* swap_chain)
{
  HADESMEM_DETAIL_TRACE_A("Detouring IDXGISwapChain.");

  try
  {
    auto const& process = GetThisProcess();
    DetourFunc(process,
               L"IDXGISwapChain::Present",
               swap_chain,
               8,
               GetIDXGISwapChainPresentDetour(),
               IDXGISwapChainPresentDetour);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

void DetourDXGIFactory(IDXGIFactory* dxgi_factory)
{
  HADESMEM_DETAIL_TRACE_A("Detouring IDXGIFactory.");

  try
  {
    auto const& process = GetThisProcess();
    DetourFunc(process,
               L"IDXGIFactory::CreateSwapChain",
               dxgi_factory,
               10,
               GetIDXGIFactoryCreateSwapChainDetour(),
               IDXGIFactoryCreateSwapChainDetour);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

void DetourDXGIFactoryFromDevice(IUnknown* device)
{
  HADESMEM_DETAIL_TRACE_A("Detouring factory from device.");

  auto const factory_wrapper = GetDXGIFactoryFromDevice(device);
  if (auto const factory = factory_wrapper.GetFactory())
  {
    switch (factory_wrapper.GetFactoryRevision())
    {
    case 0:
      HADESMEM_DETAIL_TRACE_A("Detected IDXGIFactory.");
      DetourDXGIFactory(factory);
      break;
    case 1:
      HADESMEM_DETAIL_TRACE_A("Detected IDXGIFactory1.");
      DetourDXGIFactory(static_cast<IDXGIFactory1*>(factory));
      break;
    default:
      HADESMEM_DETAIL_TRACE_A("Unknown IDXGIFactory interface.");
      HADESMEM_DETAIL_ASSERT(false);
      break;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Invalid factory pointer.");
  }
}
}
}
