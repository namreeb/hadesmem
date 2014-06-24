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
#include "dxgi.hpp"
#include "dxgi_helpers.hpp"
#include "helpers.hpp"
#include "main.hpp"
#include "module.hpp"

namespace
{

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

std::pair<void*, SIZE_T>& GetD3D11Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
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
  hadesmem::detail::DetourRefCounter ref_count{GetD3D11CreateDeviceRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%d] [%p] [%u] [%p] [%u] [%u] [%p] [%p] [%p].",
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

  auto const factory_wrapper =
    hadesmem::cerberus::GetDXGIFactoryFromDevice(*device);
  if (auto const dxgi_factory = factory_wrapper.GetFactory())
  {
    hadesmem::cerberus::DetourDXGIFactory(dxgi_factory);
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
    "Args: [%p] [%d] [%p] [%u] [%p] [%u] [%u] [%p] [%p] [%p] [%p] [%p].",
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
      hadesmem::cerberus::DetourDXGISwapChain(*swap_chain);
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid swap chain out param pointer.");
    }

    if (device)
    {
      auto const factory_wrapper =
        hadesmem::cerberus::GetDXGIFactoryFromDevice(*device);
      if (auto const dxgi_factory = factory_wrapper.GetFactory())
      {
        hadesmem::cerberus::DetourDXGIFactory(dxgi_factory);
      }
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid device out param pointer.");
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
  InitializeSupportForModule(
    L"D3D11", &DetourD3D11, &UndetourD3D11, &GetD3D11Module);
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
}
}
