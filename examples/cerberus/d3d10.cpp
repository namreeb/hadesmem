// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d10.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
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
#include "dxgi_swap_chain.hpp"
#include "helpers.hpp"
#include "main.hpp"
#include "module.hpp"

namespace
{
std::unique_ptr<hadesmem::PatchDetour>&
  GetD3D10CreateDeviceDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetD3D10CreateDeviceAndSwapChainDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetD3D10CreateDevice1Detour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetD3D10CreateDeviceAndSwapChain1Detour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetD3D10Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{};
  return module;
}

std::pair<void*, SIZE_T>& GetD3D101Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{};
  return module;
}

extern "C" HRESULT WINAPI
  D3D10CreateDeviceDetour(IDXGIAdapter* adapter,
                          D3D10_DRIVER_TYPE driver_type,
                          HMODULE software,
                          UINT flags,
                          UINT sdk_version,
                          ID3D10Device** device) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetD3D10CreateDeviceDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%d] [%p] [%u] [%u] [%p].",
                                 adapter,
                                 driver_type,
                                 software,
                                 flags,
                                 sdk_version,
                                 device);
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

  return ret;
}

extern "C" HRESULT WINAPI D3D10CreateDeviceAndSwapChainDetour(
  IDXGIAdapter* adapter,
  D3D10_DRIVER_TYPE driver_type,
  HMODULE software,
  UINT flags,
  UINT sdk_version,
  DXGI_SWAP_CHAIN_DESC* swap_chain_desc,
  IDXGISwapChain** swap_chain,
  ID3D10Device** device) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetD3D10CreateDeviceAndSwapChainDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
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
      HADESMEM_DETAIL_TRACE_A("Proxying IDXGISwapChain.");
      *swap_chain = new hadesmem::cerberus::DXGISwapChainProxy{*swap_chain};
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
  D3D10CreateDevice1Detour(IDXGIAdapter* adapter,
                           D3D10_DRIVER_TYPE driver_type,
                           HMODULE software,
                           UINT flags,
                           D3D10_FEATURE_LEVEL1 hardware_level,
                           UINT sdk_version,
                           ID3D10Device** device) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetD3D10CreateDevice1Detour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%d] [%p] [%u] [%d] [%u] [%p].",
                                 adapter,
                                 driver_type,
                                 software,
                                 flags,
                                 hardware_level,
                                 sdk_version,
                                 device);
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

  return ret;
}

extern "C" HRESULT WINAPI D3D10CreateDeviceAndSwapChain1Detour(
  IDXGIAdapter* adapter,
  D3D10_DRIVER_TYPE driver_type,
  HMODULE software,
  UINT flags,
  D3D10_FEATURE_LEVEL1 hardware_level,
  UINT sdk_version,
  DXGI_SWAP_CHAIN_DESC* swap_chain_desc,
  IDXGISwapChain** swap_chain,
  ID3D10Device** device) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetD3D10CreateDeviceAndSwapChain1Detour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
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
      HADESMEM_DETAIL_TRACE_A("Proxying IDXGISwapChain.");
      *swap_chain = new hadesmem::cerberus::DXGISwapChainProxy{*swap_chain};
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
  auto const& process = GetThisProcess();
  auto& module = GetD3D10Module();
  if (CommonDetourModule(process, L"D3D10", base, module))
  {
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
}

void UndetourD3D10(bool remove)
{
  auto& module = GetD3D10Module();
  if (CommonUndetourModule(L"D3D10", module))
  {
    UndetourFunc(L"D3D10CreateDeviceAndSwapChain",
                 GetD3D10CreateDeviceAndSwapChainDetour(),
                 remove);
    UndetourFunc(L"D3D10CreateDevice", GetD3D10CreateDeviceDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}

void DetourD3D101(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetD3D101Module();
  if (CommonDetourModule(process, L"D3D10_1", base, module))
  {
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
}

void UndetourD3D101(bool remove)
{
  auto& module = GetD3D10Module();
  if (CommonUndetourModule(L"D3D10_1", module))
  {
    UndetourFunc(L"D3D10CreateDeviceAndSwapChain1",
                 GetD3D10CreateDeviceAndSwapChain1Detour(),
                 remove);
    UndetourFunc(L"D3D10CreateDevice1", GetD3D10CreateDevice1Detour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}
}
}
