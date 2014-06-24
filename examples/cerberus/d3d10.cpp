// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d10.hpp"

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

#include <d3d10_1.h>
#include <d3d10.h>

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

std::unique_ptr<hadesmem::PatchDetour>& GetD3D10CreateDeviceDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetD3D10CreateDeviceRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetD3D10CreateDeviceAndSwapChainDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetD3D10CreateDeviceAndSwapChainRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetD3D10CreateDevice1Detour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetD3D10CreateDevice1RefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetD3D10CreateDeviceAndSwapChain1Detour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetD3D10CreateDeviceAndSwapChain1RefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::pair<void*, SIZE_T>& GetD3D10Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

std::pair<void*, SIZE_T>& GetD3D101Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

extern "C" HRESULT WINAPI D3D10CreateDeviceDetour(IDXGIAdapter* adapter,
                                                  D3D10_DRIVER_TYPE driver_type,
                                                  HMODULE software,
                                                  UINT flags,
                                                  UINT sdk_version,
                                                  ID3D10Device** device)
  HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{GetD3D10CreateDeviceRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%d] [%p] [%u] [%u] [%p].",
                                 adapter,
                                 driver_type,
                                 software,
                                 flags,
                                 sdk_version,
                                 device);
  auto& detour = GetD3D10CreateDeviceDetour();
  auto const d3d10_create_device =
    detour->GetTrampoline<decltype(&D3D10CreateDeviceDetour)>();
  last_error_preserver.Revert();
  auto const ret = d3d10_create_device(
    adapter, driver_type, software, flags, sdk_version, device);
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

extern "C" HRESULT WINAPI
  D3D10CreateDeviceAndSwapChainDetour(IDXGIAdapter* adapter,
                                      D3D10_DRIVER_TYPE driver_type,
                                      HMODULE software,
                                      UINT flags,
                                      UINT sdk_version,
                                      DXGI_SWAP_CHAIN_DESC* swap_chain_desc,
                                      IDXGISwapChain** swap_chain,
                                      ID3D10Device** device)
  HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{
    GetD3D10CreateDeviceAndSwapChainRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%d] [%p] [%u] [%u] [%p] [%p] [%p].",
    adapter,
    driver_type,
    software,
    flags,
    sdk_version,
    swap_chain_desc,
    swap_chain,
    device);
  auto& detour = GetD3D10CreateDeviceAndSwapChainDetour();
  auto const d3d10_create_device_and_swap_chain =
    detour->GetTrampoline<decltype(&D3D10CreateDeviceAndSwapChainDetour)>();
  last_error_preserver.Revert();
  auto const ret = d3d10_create_device_and_swap_chain(adapter,
                                                      driver_type,
                                                      software,
                                                      flags,
                                                      sdk_version,
                                                      swap_chain_desc,
                                                      swap_chain,
                                                      device);
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

extern "C" HRESULT WINAPI
  D3D10CreateDevice1Detour(IDXGIAdapter* adapter,
                           D3D10_DRIVER_TYPE driver_type,
                           HMODULE software,
                           UINT flags,
                           D3D10_FEATURE_LEVEL1 hardware_level,
                           UINT sdk_version,
                           ID3D10Device** device) HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{GetD3D10CreateDevice1RefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%d] [%p] [%u] [%d] [%u] [%p].",
                                 adapter,
                                 driver_type,
                                 software,
                                 flags,
                                 hardware_level,
                                 sdk_version,
                                 device);
  auto& detour = GetD3D10CreateDevice1Detour();
  auto const d3d10_create_device_1 =
    detour->GetTrampoline<decltype(&D3D10CreateDevice1Detour)>();
  last_error_preserver.Revert();
  auto const ret = d3d10_create_device_1(
    adapter, driver_type, software, flags, hardware_level, sdk_version, device);
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

extern "C" HRESULT WINAPI
  D3D10CreateDeviceAndSwapChain1Detour(IDXGIAdapter* adapter,
                                       D3D10_DRIVER_TYPE driver_type,
                                       HMODULE software,
                                       UINT flags,
                                       D3D10_FEATURE_LEVEL1 hardware_level,
                                       UINT sdk_version,
                                       DXGI_SWAP_CHAIN_DESC* swap_chain_desc,
                                       IDXGISwapChain** swap_chain,
                                       ID3D10Device** device)
  HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{
    GetD3D10CreateDeviceAndSwapChain1RefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%d] [%p] [%u] [%d] [%u] [%p] [%p] [%p].",
    adapter,
    driver_type,
    software,
    flags,
    hardware_level,
    sdk_version,
    swap_chain_desc,
    swap_chain,
    device);
  auto& detour = GetD3D10CreateDeviceAndSwapChain1Detour();
  auto const d3d10_create_device_and_swap_chain_1 =
    detour->GetTrampoline<decltype(&D3D10CreateDeviceAndSwapChain1Detour)>();
  last_error_preserver.Revert();
  auto const ret = d3d10_create_device_and_swap_chain_1(adapter,
                                                        driver_type,
                                                        software,
                                                        flags,
                                                        hardware_level,
                                                        sdk_version,
                                                        swap_chain_desc,
                                                        swap_chain,
                                                        device);
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

void InitializeD3D10()
{
  InitializeSupportForModule(
    L"D3D10", DetourD3D10, UndetourD3D10, GetD3D10Module);
}

void InitializeD3D101()
{
  InitializeSupportForModule(
    L"D3D10_1", DetourD3D101, UndetourD3D101, GetD3D101Module);
}

void DetourD3D10(HMODULE base)
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  auto& module = GetD3D10Module();
  if (module.first)
  {
    HADESMEM_DETAIL_TRACE_A("D3D10 already detoured.");
    return;
  }

  if (!base)
  {
    base = ::GetModuleHandleW(L"d3d10");
  }

  if (!base)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to find D3D10 module.");
    return;
  }

  auto const& process = GetThisProcess();

  module =
    std::make_pair(base, hadesmem::detail::GetRegionAllocSize(process, base));

  DetourFunc(process,
             base,
             "D3D10CreateDevice",
             GetD3D10CreateDeviceDetour(),
             D3D10CreateDeviceDetour);
  DetourFunc(process,
             base,
             "D3D10CreateDeviceAndSwapChain",
             GetD3D10CreateDeviceAndSwapChainDetour(),
             D3D10CreateDeviceAndSwapChainDetour);
}

void UndetourD3D10(bool remove)
{
  auto& module = GetD3D10Module();
  if (!module.first)
  {
    HADESMEM_DETAIL_TRACE_A("D3D10 not detoured.");
    return;
  }

  UndetourFunc(L"D3D10CreateDeviceAndSwapChain",
               GetD3D10CreateDeviceAndSwapChainDetour(),
               GetD3D10CreateDeviceAndSwapChainRefCount(),
               remove);
  UndetourFunc(L"D3D10CreateDevice",
               GetD3D10CreateDeviceDetour(),
               GetD3D10CreateDeviceRefCount(),
               remove);

  module = std::make_pair(nullptr, 0);
}

void DetourD3D101(HMODULE base)
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  auto& module = GetD3D101Module();
  if (module.first)
  {
    HADESMEM_DETAIL_TRACE_A("D3D10_1 already detoured.");
    return;
  }

  if (!base)
  {
    base = ::GetModuleHandleW(L"d3d10_1");
  }

  if (!base)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to find D3D10_1 module.");
    return;
  }

  auto const& process = GetThisProcess();

  module =
    std::make_pair(base, hadesmem::detail::GetRegionAllocSize(process, base));

  DetourFunc(process,
             base,
             "D3D10CreateDevice1",
             GetD3D10CreateDevice1Detour(),
             D3D10CreateDevice1Detour);
  DetourFunc(process,
             base,
             "D3D10CreateDeviceAndSwapChain1",
             GetD3D10CreateDeviceAndSwapChain1Detour(),
             D3D10CreateDeviceAndSwapChain1Detour);
}

void UndetourD3D101(bool remove)
{
  auto& module = GetD3D10Module();
  if (!module.first)
  {
    HADESMEM_DETAIL_TRACE_A("D3D10_1 not detoured.");
    return;
  }

  UndetourFunc(L"D3D10CreateDeviceAndSwapChain1",
               GetD3D10CreateDeviceAndSwapChain1Detour(),
               GetD3D10CreateDeviceAndSwapChain1RefCount(),
               remove);
  UndetourFunc(L"D3D10CreateDevice1",
               GetD3D10CreateDevice1Detour(),
               GetD3D10CreateDevice1RefCount(),
               remove);

  module = std::make_pair(nullptr, 0);
}
}
}
