// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "process.hpp"

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
#include <hadesmem/detail/last_error.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "detour_ref_counter.hpp"
#include "main.hpp"

namespace
{

ID3D11Device*& GetDevice()
{
  static ID3D11Device* device;
  return device;
}

ID3D11DeviceContext*& GetDeviceContext()
{
  static ID3D11DeviceContext* device_context;
  return device_context;
}

std::unique_ptr<hadesmem::PatchDetour>& GetIDXGISwapChainPresentDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetIDXGISwapChainPresentRefCount()
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetIDXGIFactoryCreateSwapChainDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetIDXGIFactoryCreateSwapChainRefCount()
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetD3D11CreateDeviceDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetD3D11CreateDeviceRefCount()
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetD3D11CreateDeviceAndSwapChainDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetD3D11CreateDeviceAndSwapChainRefCount()
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetCreateDXGIFactoryDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetCreateDXGIFactoryRefCount()
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

// Modified version of code from http://bit.ly/1iizOJR.
extern "C" HRESULT WINAPI
  IDXGISwapChainPresentDetour(IDXGISwapChain* swap_chain,
                              UINT sync_interval,
                              UINT flags)
{
  DetourRefCounter ref_count{GetIDXGISwapChainPresentRefCount()};

  hadesmem::detail::LastErrorPreserver last_error;
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%u] [%u].", swap_chain, sync_interval, flags);
  auto& detour = GetIDXGISwapChainPresentDetour();
  auto const present =
    detour->GetTrampoline<decltype(&IDXGISwapChainPresentDetour)>();

  static std::once_flag once;
  auto const init = [&]()
  {
    auto& device = GetDevice();
    auto& device_context = GetDeviceContext();
    if (SUCCEEDED(swap_chain->GetDevice(__uuidof(device),
                                        reinterpret_cast<void**>(&device))))
    {
      device->GetImmediateContext(&device_context);

      // Put init code here.
    }
  };
  std::call_once(once, init);

  // Put drawing code here.

  last_error.Revert();
  auto const ret = present(swap_chain, sync_interval, flags);
  last_error.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);
  return ret;
}

void HookDXGISwapChain(IDXGISwapChain* swap_chain)
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
{
  DetourRefCounter ref_count{GetIDXGIFactoryCreateSwapChainRefCount()};

  hadesmem::detail::LastErrorPreserver last_error;
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%p] [%p] [%p].", factory, device, desc, swap_chain);
  auto& detour = GetIDXGIFactoryCreateSwapChainDetour();
  auto const create_swap_chain =
    detour->GetTrampoline<decltype(&IDXGIFactoryCreateSwapChainDetour)>();
  last_error.Revert();
  auto const ret = create_swap_chain(factory, device, desc, swap_chain);
  last_error.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    HookDXGISwapChain(*swap_chain);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

void HookDXGIFactory(IDXGIFactory* dxgi_factory)
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
{
  DetourRefCounter ref_count{GetD3D11CreateDeviceRefCount()};

  hadesmem::detail::LastErrorPreserver last_error;
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
  last_error.Revert();
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
  last_error.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    IDXGIDevice* dxgi_device;
    if (SUCCEEDED((*device)->QueryInterface(__uuidof(IDXGIDevice),
                                            (void**)&dxgi_device)))
    {
      IDXGIAdapter* dxgi_adapter;
      if (SUCCEEDED(dxgi_device->GetParent(__uuidof(IDXGIAdapter),
                                           (void**)&dxgi_adapter)))
      {
        IDXGIFactory* dxgi_factory;
        if (SUCCEEDED(dxgi_adapter->GetParent(__uuidof(IDXGIFactory),
                                              (void**)&dxgi_factory)))
        {
          HookDXGIFactory(dxgi_factory);
        }
        else
        {
          HADESMEM_DETAIL_TRACE_A("Failed to get IDXGIFactory.");
        }
      }
      else
      {
        HADESMEM_DETAIL_TRACE_A("Failed to get IDXGIAdapter.");
      }
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Failed to get IDXGIDevice.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
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
  ID3D11DeviceContext** immediate_context)
{
  DetourRefCounter ref_count{GetD3D11CreateDeviceAndSwapChainRefCount()};

  hadesmem::detail::LastErrorPreserver last_error;
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
  last_error.Revert();
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
  last_error.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    HookDXGISwapChain(*swap_chain);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

extern "C" HRESULT WINAPI CreateDXGIFactoryDetour(REFIID riid, void** factory)
{
  DetourRefCounter ref_count{GetCreateDXGIFactoryRefCount()};

  hadesmem::detail::LastErrorPreserver last_error;
  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p].", &riid, factory);
  auto& detour = GetCreateDXGIFactoryDetour();
  auto const create_dxgi_factory =
    detour->GetTrampoline<decltype(&CreateDXGIFactoryDetour)>();
  last_error.Revert();
  auto const ret = create_dxgi_factory(riid, factory);
  last_error.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (riid == __uuidof(IDXGIFactory))
    {
      HADESMEM_DETAIL_TRACE_A("Hooking IDXGIFactory.");

      HookDXGIFactory(static_cast<IDXGIFactory*>(*factory));
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

void DetourD3D11(HMODULE base)
{
  if (!base)
  {
    base = ::GetModuleHandleW(L"d3d11");
  }

  if (!base)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to find D3D11 module.");
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
  if (!base)
  {
    base = ::GetModuleHandleW(L"DXGI");
  }

  if (!base)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to find DXGI module.");
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

void UndetourD3D11()
{
  if (GetD3D11CreateDeviceAndSwapChainDetour())
  {
    auto& detour = GetD3D11CreateDeviceAndSwapChainDetour();
    detour->Remove();
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
    detour->Remove();
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
}

void UndetourDXGI()
{
  if (GetCreateDXGIFactoryDetour())
  {
    auto& detour = GetCreateDXGIFactoryDetour();
    detour->Remove();
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
    detour->Remove();
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
    detour->Remove();
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
}
