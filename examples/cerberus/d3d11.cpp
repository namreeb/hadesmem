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
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>

#include "callbacks.hpp"
#include "detour_ref_counter.hpp"
#include "main.hpp"
#include "module.hpp"

namespace
{

ID3D11Device*& GetDevice() HADESMEM_DETAIL_NOEXCEPT
{
  static ID3D11Device* device;
  return device;
}

ID3D11DeviceContext*& GetDeviceContext() HADESMEM_DETAIL_NOEXCEPT
{
  static ID3D11DeviceContext* device_context;
  return device_context;
}

std::unique_ptr<hadesmem::PatchDetour>& GetIDXGISwapChainPresentDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetIDXGISwapChainPresentRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetIDXGIFactoryCreateSwapChainDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetIDXGIFactoryCreateSwapChainRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetD3D11CreateDeviceDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetD3D11CreateDeviceRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetD3D11CreateDeviceAndSwapChainDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetD3D11CreateDeviceAndSwapChainRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetCreateDXGIFactoryDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetCreateDXGIFactoryRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

Callbacks<OnFrameCallback>& GetOnFrameCallbacks()
{
  static Callbacks<OnFrameCallback> callbacks;
  return callbacks;
}

// Modified version of code from http://bit.ly/1iizOJR.
extern "C" HRESULT WINAPI
  IDXGISwapChainPresentDetour(IDXGISwapChain* swap_chain,
                              UINT sync_interval,
                              UINT flags) HADESMEM_DETAIL_NOEXCEPT
{
  DetourRefCounter ref_count{GetIDXGISwapChainPresentRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%u] [%u].", swap_chain, sync_interval, flags);
  auto& detour = GetIDXGISwapChainPresentDetour();
  auto const present =
    detour->GetTrampoline<decltype(&IDXGISwapChainPresentDetour)>();

  static std::once_flag once;
  auto const init = [&]()
  {
    HADESMEM_DETAIL_TRACE_A("Performing initialization.");

    auto& device = GetDevice();
    auto& device_context = GetDeviceContext();
    if (FAILED(swap_chain->GetDevice(__uuidof(device),
                                     reinterpret_cast<void**>(&device))))
    {
      HADESMEM_DETAIL_TRACE_A("WARNING! Failed to get device from swap chain.");
      return;
    }

    device->GetImmediateContext(&device_context);

    // Put init code here.
  };
  std::call_once(once, init);

  auto& callbacks = GetOnFrameCallbacks();
  callbacks.Run(swap_chain, GetDevice(), GetDeviceContext());

  last_error_preserver.Revert();
  auto const ret = present(swap_chain, sync_interval, flags);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);
  return ret;
}

void DetourDXGISwapChain(IDXGISwapChain* swap_chain)
{
  void** const swap_chain_vtable = *reinterpret_cast<void***>(swap_chain);
  auto const present = swap_chain_vtable[8];
  auto const present_detour_fn =
    hadesmem::detail::UnionCast<void*>(&IDXGISwapChainPresentDetour);
  auto& present_detour = GetIDXGISwapChainPresentDetour();
  if (!present_detour)
  {
    present_detour = std::make_unique<hadesmem::PatchDetour>(
      GetThisProcess(), present, present_detour_fn);
    present_detour->Apply();
    HADESMEM_DETAIL_TRACE_A("IDXGISwapChain::Present detoured.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("IDXGISwapChain::Present already detoured.");
  }
}

extern "C" HRESULT WINAPI
  IDXGIFactoryCreateSwapChainDetour(IDXGIFactory* factory,
                                    IUnknown* device,
                                    DXGI_SWAP_CHAIN_DESC* desc,
                                    IDXGISwapChain** swap_chain)
  HADESMEM_DETAIL_NOEXCEPT
{
  DetourRefCounter ref_count{GetIDXGIFactoryCreateSwapChainRefCount()};
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

    DetourDXGISwapChain(*swap_chain);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

void DetourDXGIFactory(IDXGIFactory* dxgi_factory)
{
  void** const dxgi_factory_vtable = *reinterpret_cast<void***>(dxgi_factory);
  auto const create_swap_chain = dxgi_factory_vtable[10];
  auto const create_swap_chain_detour_fn =
    hadesmem::detail::UnionCast<void*>(&IDXGIFactoryCreateSwapChainDetour);
  auto& create_swap_chain_detour = GetIDXGIFactoryCreateSwapChainDetour();
  if (!create_swap_chain_detour)
  {
    create_swap_chain_detour = std::make_unique<hadesmem::PatchDetour>(
      GetThisProcess(), create_swap_chain, create_swap_chain_detour_fn);
    create_swap_chain_detour->Apply();
    HADESMEM_DETAIL_TRACE_A("IDXGIFactory::CreateSwapChain detoured.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("IDXGIFactory::CreateSwapChain already detoured.");
  }
}

extern "C" HRESULT WINAPI
  D3D11CreateDeviceDetour(IDXGIAdapter* adapter,
                          D3D_DRIVER_TYPE driver_type,
                          HMODULE software,
                          UINT flags,
                          const D3D_FEATURE_LEVEL* ptr_feature_levels,
                          UINT feature_levels,
                          UINT sdk_version,
                          ID3D11Device** device,
                          D3D_FEATURE_LEVEL* feature_level,
                          ID3D11DeviceContext** immediate_context)
  HADESMEM_DETAIL_NOEXCEPT
{
  DetourRefCounter ref_count{GetD3D11CreateDeviceRefCount()};
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

  IDXGIDevice* dxgi_device = nullptr;
  if (FAILED((*device)->QueryInterface(__uuidof(IDXGIDevice),
                                       reinterpret_cast<void**>(&dxgi_device))))
  {
    HADESMEM_DETAIL_TRACE_A("Failed to get IDXGIDevice.");
    return ret;
  }

  IDXGIAdapter* dxgi_adapter = nullptr;
  if (FAILED(dxgi_device->GetParent(__uuidof(IDXGIAdapter),
                                    reinterpret_cast<void**>(&dxgi_adapter))))
  {
    HADESMEM_DETAIL_TRACE_A("Failed to get IDXGIAdapter.");
    return ret;
  }

  IDXGIFactory* dxgi_factory = nullptr;
  if (FAILED(dxgi_adapter->GetParent(__uuidof(IDXGIFactory),
                                     reinterpret_cast<void**>(&dxgi_factory))))
  {
    HADESMEM_DETAIL_TRACE_A("Failed to get IDXGIFactory.");
    return ret;
  }

  DetourDXGIFactory(dxgi_factory);

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
  DetourRefCounter ref_count{GetD3D11CreateDeviceAndSwapChainRefCount()};
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

    DetourDXGISwapChain(*swap_chain);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

extern "C" HRESULT WINAPI CreateDXGIFactoryDetour(REFIID riid, void** factory)
  HADESMEM_DETAIL_NOEXCEPT
{
  DetourRefCounter ref_count{GetCreateDXGIFactoryRefCount()};
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
    auto const dxgi_mod_beg = d3d11_mod.first;
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
  if (GetD3D11Module().first)
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

  try
  {
    hadesmem::Region region(GetThisProcess(), base);
    GetD3D11Module() = std::make_pair(region.GetBase(), region.GetSize());
  }
  catch (std::exception const& /*e*/)
  {
    HADESMEM_DETAIL_ASSERT(false);
    return;
  }

  if (!GetD3D11CreateDeviceDetour())
  {
    auto const orig_fn = hadesmem::detail::GetProcAddressInternal(
      GetThisProcess(), base, "D3D11CreateDevice");
    if (orig_fn)
    {
      auto const detour_fn =
        hadesmem::detail::UnionCast<void*>(&D3D11CreateDeviceDetour);
      auto& detour = GetD3D11CreateDeviceDetour();
      detour = std::make_unique<hadesmem::PatchDetour>(
        GetThisProcess(), orig_fn, detour_fn);
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
    auto const orig_fn = hadesmem::detail::GetProcAddressInternal(
      GetThisProcess(), base, "D3D11CreateDeviceAndSwapChain");
    if (orig_fn)
    {
      auto const detour_fn = hadesmem::detail::UnionCast<void*>(
        &D3D11CreateDeviceAndSwapChainDetour);
      auto& detour = GetD3D11CreateDeviceAndSwapChainDetour();
      detour = std::make_unique<hadesmem::PatchDetour>(
        GetThisProcess(), orig_fn, detour_fn);
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
  if (GetDXGIModule().first)
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

  try
  {
    hadesmem::Region region(GetThisProcess(), base);
    GetDXGIModule() = std::make_pair(region.GetBase(), region.GetSize());
  }
  catch (std::exception const& /*e*/)
  {
    HADESMEM_DETAIL_ASSERT(false);
    return;
  }

  if (!GetCreateDXGIFactoryDetour())
  {
    auto const orig_fn = hadesmem::detail::GetProcAddressInternal(
      GetThisProcess(), base, "CreateDXGIFactory");
    if (orig_fn)
    {
      auto const detour_fn =
        hadesmem::detail::UnionCast<void*>(&CreateDXGIFactoryDetour);
      auto& detour = GetCreateDXGIFactoryDetour();
      detour = std::make_unique<hadesmem::PatchDetour>(
        GetThisProcess(), orig_fn, detour_fn);
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
  if (!GetD3D11Module().first)
  {
    HADESMEM_DETAIL_TRACE_A("D3D11 not detoured.");
    return;
  }

  if (GetD3D11CreateDeviceAndSwapChainDetour())
  {
    auto& detour = GetD3D11CreateDeviceAndSwapChainDetour();
    remove ? detour->Remove() : detour->Detach();
    HADESMEM_DETAIL_TRACE_A("D3D11CreateDeviceAndSwapChain undetoured.");
    detour = nullptr;

    auto& ref_count = GetD3D11CreateDeviceAndSwapChainRefCount();
    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_A(
        "Spinning on D3D11CreateDeviceAndSwapChain ref count.");
    }
    HADESMEM_DETAIL_TRACE_A(
      "D3D11CreateDeviceAndSwapChain free of references.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A(
      "D3D11CreateDeviceAndSwapChain not detoured. Skipping.");
  }

  if (GetD3D11CreateDeviceDetour())
  {
    auto& detour = GetD3D11CreateDeviceDetour();
    remove ? detour->Remove() : detour->Detach();
    HADESMEM_DETAIL_TRACE_A("D3D11CreateDevice undetoured.");
    detour = nullptr;

    auto& ref_count = GetD3D11CreateDeviceRefCount();
    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_A("Spinning on D3D11CreateDevice ref count.");
    }
    HADESMEM_DETAIL_TRACE_A("D3D11CreateDevice free of references.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("D3D11CreateDevice not detoured. Skipping.");
  }

  GetD3D11Module() = std::make_pair(nullptr, 0);
}

void UndetourDXGI(bool remove)
{
  if (!GetDXGIModule().first)
  {
    HADESMEM_DETAIL_TRACE_A("DXGI not detoured.");
    return;
  }

  if (GetCreateDXGIFactoryDetour())
  {
    auto& detour = GetCreateDXGIFactoryDetour();
    remove ? detour->Remove() : detour->Detach();
    HADESMEM_DETAIL_TRACE_A("CreateDXGIFactory undetoured.");
    detour = nullptr;

    auto& ref_count = GetCreateDXGIFactoryRefCount();
    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_A("Spinning on CreateDXGIFactory ref count.");
    }
    HADESMEM_DETAIL_TRACE_A("CreateDXGIFactory free of references.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("CreateDXGIFactory not detoured. Skipping.");
  }

  if (GetIDXGIFactoryCreateSwapChainDetour())
  {
    auto& detour = GetIDXGIFactoryCreateSwapChainDetour();
    remove ? detour->Remove() : detour->Detach();
    HADESMEM_DETAIL_TRACE_A("IDXGIFactoryCreateSwapChain undetoured.");
    detour = nullptr;

    auto& ref_count = GetIDXGIFactoryCreateSwapChainRefCount();
    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_A(
        "Spinning on IDXGIFactoryCreateSwapChain ref count.");
    }
    HADESMEM_DETAIL_TRACE_A("IDXGIFactoryCreateSwapChain free of references.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A(
      "IDXGIFactoryCreateSwapChain not detoured. Skipping.");
  }

  if (GetIDXGISwapChainPresentDetour())
  {
    auto& detour = GetIDXGISwapChainPresentDetour();
    remove ? detour->Remove() : detour->Detach();
    HADESMEM_DETAIL_TRACE_A("IDXGISwapChain::Present undetoured.");
    detour = nullptr;

    auto& ref_count = GetIDXGISwapChainPresentRefCount();
    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_A("Spinning on IDXGISwapChain::Present ref count.");
    }
    HADESMEM_DETAIL_TRACE_A("IDXGISwapChain::Present free of references.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("IDXGISwapChain::Present not detoured. Skipping.");
  }

  GetDXGIModule() = std::make_pair(nullptr, 0);
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

std::size_t RegisterOnFrameCallback(std::function<OnFrameCallback> const& callback)
{
  auto& callbacks = GetOnFrameCallbacks();
  return callbacks.Register(callback);
}

void UnregisterOnFrameCallback(std::size_t id)
{
  auto& callbacks = GetOnFrameCallbacks();
  return callbacks.Unregister(id);
}
