// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d11.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <d3d11.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/detour_ref_counter.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>

#include "callbacks.hpp"
#include "helper.hpp"
#include "main.hpp"
#include "module.hpp"

namespace
{

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDXGISwapChainPresentDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>&
  GetIDXGISwapChainPresentRefCount() HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDXGIFactoryCreateSwapChainDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>&
  GetIDXGIFactoryCreateSwapChainRefCount() HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetD3D11CreateDeviceDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>&
  GetD3D11CreateDeviceRefCount() HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetD3D11CreateDeviceAndSwapChainDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>&
  GetD3D11CreateDeviceAndSwapChainRefCount() HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetCreateDXGIFactoryDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>&
  GetCreateDXGIFactoryRefCount() HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::pair<void*, SIZE_T>& GetD3D11Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

std::pair<void*, SIZE_T>& GetDXGIModule() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackD3D11>&
  GetOnFrameCallbacksD3D11()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackD3D11>
    callbacks;
  return callbacks;
}

// Modified version of code from http://bit.ly/1iizOJR.
extern "C" HRESULT WINAPI
  IDXGISwapChainPresentDetour(IDXGISwapChain* swap_chain,
                              UINT sync_interval,
                              UINT flags) HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{
    GetIDXGISwapChainPresentRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%u] [%u].", swap_chain, sync_interval, flags);

  try
  {
    auto& callbacks = GetOnFrameCallbacksD3D11();
    callbacks.Run(swap_chain);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  auto& detour = GetIDXGISwapChainPresentDetour();
  auto const present =
    detour->GetTrampoline<decltype(&IDXGISwapChainPresentDetour)>();
  last_error_preserver.Revert();
  auto const ret = present(swap_chain, sync_interval, flags);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);
  return ret;
}

void DetourDXGISwapChain(IDXGISwapChain* swap_chain)
{
  try
  {
    auto& present_detour = GetIDXGISwapChainPresentDetour();
    if (!present_detour)
    {
      void** const swap_chain_vtable = *reinterpret_cast<void***>(swap_chain);
      auto const present = swap_chain_vtable[8];
      auto const present_detour_fn =
        hadesmem::detail::UnionCast<void*>(&IDXGISwapChainPresentDetour);
      present_detour = std::make_unique<hadesmem::PatchDetour>(
        hadesmem::cerberus::GetThisProcess(), present, present_detour_fn);
      present_detour->Apply();
      HADESMEM_DETAIL_TRACE_A("IDXGISwapChain::Present detoured.");
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("IDXGISwapChain::Present already detoured.");
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

extern "C" HRESULT WINAPI IDXGIFactoryCreateSwapChainDetour(
  IDXGIFactory* factory,
  IUnknown* device,
  DXGI_SWAP_CHAIN_DESC* desc,
  IDXGISwapChain** swap_chain) HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{
    GetIDXGIFactoryCreateSwapChainRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%p] [%p] [%p].", factory, device, desc, swap_chain);
  auto& detour = GetIDXGIFactoryCreateSwapChainDetour();
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
      DetourDXGISwapChain(*swap_chain);
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

void DetourDXGIFactory(IDXGIFactory* dxgi_factory)
{
  try
  {
    auto& create_swap_chain_detour = GetIDXGIFactoryCreateSwapChainDetour();
    if (!create_swap_chain_detour)
    {
      void** const dxgi_factory_vtable =
        *reinterpret_cast<void***>(dxgi_factory);
      auto const create_swap_chain = dxgi_factory_vtable[10];
      auto const create_swap_chain_detour_fn =
        hadesmem::detail::UnionCast<void*>(&IDXGIFactoryCreateSwapChainDetour);
      create_swap_chain_detour = std::make_unique<hadesmem::PatchDetour>(
        hadesmem::cerberus::GetThisProcess(),
        create_swap_chain,
        create_swap_chain_detour_fn);
      create_swap_chain_detour->Apply();
      HADESMEM_DETAIL_TRACE_A("IDXGIFactory::CreateSwapChain detoured.");
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A(
        "IDXGIFactory::CreateSwapChain already detoured.");
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

extern "C" HRESULT WINAPI D3D11CreateDeviceDetour(
  IDXGIAdapter* adapter,
  D3D_DRIVER_TYPE driver_type,
  HMODULE software,
  UINT flags,
  const D3D_FEATURE_LEVEL* ptr_feature_levels,
  UINT feature_levels,
  UINT sdk_version,
  ID3D11Device** device,
  D3D_FEATURE_LEVEL* feature_level,
  ID3D11DeviceContext** immediate_context) HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{GetD3D11CreateDeviceRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%d] [%p] [%u] [%p] [%u] [%u] [%p] [%p] [%p].",
    adapter,
    driver_type,
    software,
    flags,
    ptr_feature_levels,
    feature_levels,
    sdk_version,
    device,
    feature_level,
    immediate_context);
  auto& detour = GetD3D11CreateDeviceDetour();
  auto const d3d11_create_device =
    detour->GetTrampoline<decltype(&D3D11CreateDeviceDetour)>();
  last_error_preserver.Revert();
  auto const ret = d3d11_create_device(adapter,
                                       driver_type,
                                       software,
                                       flags,
                                       ptr_feature_levels,
                                       feature_levels,
                                       sdk_version,
                                       device,
                                       feature_level,
                                       immediate_context);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (FAILED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
    return ret;
  }

  HADESMEM_DETAIL_TRACE_A("Succeeded.");

  if (!device)
  {
    HADESMEM_DETAIL_TRACE_A("Invalid device out param pointer.");
    return ret;
  }

  IDXGIDevice* dxgi_device = nullptr;
  IDXGIAdapter* dxgi_adapter = nullptr;
  IDXGIFactory* dxgi_factory = nullptr;

  try
  {
    if (FAILED((*device)->QueryInterface(
          __uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device))))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
                               "ID3D11Device::QueryInterface failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    if (FAILED(dxgi_device->GetParent(__uuidof(IDXGIAdapter),
                                      reinterpret_cast<void**>(&dxgi_adapter))))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
                               "IDXGIDevice::GetParent failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    if (FAILED(dxgi_adapter->GetParent(
          __uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgi_factory))))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
                               "IDXGIAdapter::GetParent failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    DetourDXGIFactory(dxgi_factory);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  if (dxgi_factory)
  {
    dxgi_factory->Release();
  }

  if (dxgi_adapter)
  {
    dxgi_adapter->Release();
  }

  if (dxgi_device)
  {
    dxgi_device->Release();
  }

  return ret;
}

extern "C" HRESULT WINAPI D3D11CreateDeviceAndSwapChainDetour(
  IDXGIAdapter* adapter,
  D3D_DRIVER_TYPE driver_type,
  HMODULE software,
  UINT flags,
  const D3D_FEATURE_LEVEL* ptr_feature_levels,
  UINT feature_levels,
  UINT sdk_version,
  const DXGI_SWAP_CHAIN_DESC* swap_chain_desc,
  IDXGISwapChain** swap_chain,
  ID3D11Device** device,
  D3D_FEATURE_LEVEL* feature_level,
  ID3D11DeviceContext** immediate_context) HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{
    GetD3D11CreateDeviceAndSwapChainRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%d] [%p] [%u] [%p] [%u] [%u] [%p] [%p] [%p] [%p] [%p].",
    adapter,
    driver_type,
    software,
    flags,
    ptr_feature_levels,
    feature_levels,
    sdk_version,
    swap_chain_desc,
    swap_chain,
    device,
    feature_level,
    immediate_context);
  auto& detour = GetD3D11CreateDeviceAndSwapChainDetour();
  auto const d3d11_create_device_and_swap_chain =
    detour->GetTrampoline<decltype(&D3D11CreateDeviceAndSwapChainDetour)>();
  last_error_preserver.Revert();
  auto const ret = d3d11_create_device_and_swap_chain(adapter,
                                                      driver_type,
                                                      software,
                                                      flags,
                                                      ptr_feature_levels,
                                                      feature_levels,
                                                      sdk_version,
                                                      swap_chain_desc,
                                                      swap_chain,
                                                      device,
                                                      feature_level,
                                                      immediate_context);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (swap_chain)
    {
      DetourDXGISwapChain(*swap_chain);
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

extern "C" HRESULT WINAPI
  CreateDXGIFactoryDetour(REFIID riid, void** factory) HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{GetCreateDXGIFactoryRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto& detour = GetCreateDXGIFactoryDetour();
  auto const create_dxgi_factory =
    detour->GetTrampoline<decltype(&CreateDXGIFactoryDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_dxgi_factory(riid, factory);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (riid == __uuidof(IDXGIFactory))
    {
      HADESMEM_DETAIL_TRACE_A("Detouring IDXGIFactory.");

      DetourDXGIFactory(static_cast<IDXGIFactory*>(*factory));
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Unsupported GUID.");
    }
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

void InitializeD3D11()
{
  HADESMEM_DETAIL_TRACE_A("Initializing D3D11.");

  auto const on_map = [](
    HMODULE module, std::wstring const& /*path*/, std::wstring const& name)
  {
    if (name == L"D3D11" || name == L"D3D11.DLL")
    {
      HADESMEM_DETAIL_TRACE_A("D3D11 loaded. Applying hooks.");

      DetourD3D11(module);
    }
    else if (name == L"DXGI" || name == L"DXGI.DLL")
    {
      HADESMEM_DETAIL_TRACE_A("DXGI loaded. Applying hooks.");

      DetourDXGI(module);
    }
  };
  RegisterOnMapCallback(on_map);
  HADESMEM_DETAIL_TRACE_A("Registered for OnMap.");

  auto const on_unmap = [](HMODULE module)
  {
    auto const d3d11_mod = GetD3D11Module();
    auto const d3d11_mod_beg = d3d11_mod.first;
    void* const d3d11_mod_end =
      static_cast<std::uint8_t*>(d3d11_mod.first) + d3d11_mod.second;
    if (module >= d3d11_mod_beg && module < d3d11_mod_end)
    {
      HADESMEM_DETAIL_TRACE_A("D3D11 unloaded. Removing hooks.");

      // Detach instead of remove hooks because when we get the notification the
      // memory region is already gone.
      UndetourD3D11(false);
    }

    auto const dxgi_mod = GetDXGIModule();
    auto const dxgi_mod_beg = dxgi_mod.first;
    void* const dxgi_mod_end =
      static_cast<std::uint8_t*>(dxgi_mod.first) + dxgi_mod.second;
    if (module >= dxgi_mod_beg && module < dxgi_mod_end)
    {
      HADESMEM_DETAIL_TRACE_A("DXGI unloaded. Removing hooks.");

      // Detach instead of remove hooks because when we get the notification the
      // memory region is already gone.
      UndetourDXGI(false);
    }
  };
  RegisterOnUnmapCallback(on_unmap);
  HADESMEM_DETAIL_TRACE_A("Registered for OnUnmap.");
}

void DetourD3D11(HMODULE base)
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  auto& module = GetD3D11Module();
  if (module.first)
  {
    HADESMEM_DETAIL_TRACE_A("D3D11 already detoured.");
    return;
  }

  if (!base)
  {
    base = ::GetModuleHandleW(L"d3d11");
  }

  if (!base)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to find D3D11 module.");
    return;
  }

  auto const& process = GetThisProcess();

  module =
    std::make_pair(base, hadesmem::detail::GetRegionAllocSize(process, base));

  if (!GetD3D11CreateDeviceDetour())
  {
    auto const orig_fn =
      detail::GetProcAddressInternal(process, base, "D3D11CreateDevice");
    if (orig_fn)
    {
      auto const detour_fn = detail::UnionCast<void*>(&D3D11CreateDeviceDetour);
      auto& detour = GetD3D11CreateDeviceDetour();
      detour.reset(new PatchDetour(process, orig_fn, detour_fn));
      detour->Apply();
      HADESMEM_DETAIL_TRACE_A("D3D11CreateDevice detoured.");
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Could not find D3D11CreateDevice export.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("D3D11CreateDevice already detoured.");
  }

  if (!GetD3D11CreateDeviceAndSwapChainDetour())
  {
    auto const orig_fn = detail::GetProcAddressInternal(
      process, base, "D3D11CreateDeviceAndSwapChain");
    if (orig_fn)
    {
      auto const detour_fn =
        detail::UnionCast<void*>(&D3D11CreateDeviceAndSwapChainDetour);
      auto& detour = GetD3D11CreateDeviceAndSwapChainDetour();
      detour.reset(new PatchDetour(process, orig_fn, detour_fn));
      detour->Apply();
      HADESMEM_DETAIL_TRACE_A("D3D11CreateDeviceAndSwapChain detoured.");
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A(
        "Could not find D3D11CreateDeviceAndSwapChain export.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("D3D11CreateDeviceAndSwapChain already detoured.");
  }
}

void DetourDXGI(HMODULE base)
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  auto& module = GetDXGIModule();
  if (module.first)
  {
    HADESMEM_DETAIL_TRACE_A("DXGI already detoured.");
    return;
  }

  if (!base)
  {
    base = ::GetModuleHandleW(L"DXGI");
  }

  if (!base)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to find DXGI module.");
    return;
  }

  auto const& process = GetThisProcess();

  module =
    std::make_pair(base, hadesmem::detail::GetRegionAllocSize(process, base));

  if (!GetCreateDXGIFactoryDetour())
  {
    auto const orig_fn =
      detail::GetProcAddressInternal(process, base, "CreateDXGIFactory");
    if (orig_fn)
    {
      auto const detour_fn = detail::UnionCast<void*>(&CreateDXGIFactoryDetour);
      auto& detour = GetCreateDXGIFactoryDetour();
      detour.reset(new PatchDetour(process, orig_fn, detour_fn));
      detour->Apply();
      HADESMEM_DETAIL_TRACE_A("CreateDXGIFactory detoured.");
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Could not find CreateDXGIFactory export.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("CreateDXGIFactory already detoured.");
  }
}

void UndetourD3D11(bool remove)
{
  auto& module = GetD3D11Module();
  if (!module.first)
  {
    HADESMEM_DETAIL_TRACE_A("D3D11 not detoured.");
    return;
  }

  UndetourFunc(L"D3D11CreateDeviceAndSwapChain",
               GetD3D11CreateDeviceAndSwapChainDetour(),
               GetD3D11CreateDeviceAndSwapChainRefCount(),
               remove);
  UndetourFunc(L"D3D11CreateDevice",
               GetD3D11CreateDeviceDetour(),
               GetD3D11CreateDeviceRefCount(),
               remove);

  module = std::make_pair(nullptr, 0);
}

void UndetourDXGI(bool remove)
{
  auto& module = GetDXGIModule();
  if (!module.first)
  {
    HADESMEM_DETAIL_TRACE_A("DXGI not detoured.");
    return;
  }

  UndetourFunc(L"CreateDXGIFactory",
               GetCreateDXGIFactoryDetour(),
               GetCreateDXGIFactoryRefCount(),
               remove);
  UndetourFunc(L"IDXGIFactory::CreateSwapChain",
               GetIDXGIFactoryCreateSwapChainDetour(),
               GetIDXGIFactoryCreateSwapChainRefCount(),
               remove);
  UndetourFunc(L"IDXGISwapChain::Present",
               GetIDXGISwapChainPresentDetour(),
               GetIDXGISwapChainPresentRefCount(),
               remove);

  module = std::make_pair(nullptr, 0);
}

std::size_t RegisterOnFrameCallbackD3D11(
  std::function<OnFrameCallbackD3D11> const& callback)
{
  auto& callbacks = GetOnFrameCallbacksD3D11();
  return callbacks.Register(callback);
}

void UnregisterOnFrameCallbackD3D11(std::size_t id)
{
  auto& callbacks = GetOnFrameCallbacksD3D11();
  return callbacks.Unregister(id);
}
}
}
