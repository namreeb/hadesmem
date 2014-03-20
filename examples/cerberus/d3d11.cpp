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
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "detour_ref_counter.hpp"
#include "main.hpp"

// Modified version of code from http://bit.ly/1iizOJR.

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_device_context = nullptr;

namespace
{

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

std::unique_ptr<hadesmem::PatchDetour>&
  GetID3D11DeviceContextDrawIndexedDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetID3D11DeviceContextDrawIndexedRefCount()
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetID3D11DeviceContextClearRenderTargetViewDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>&
  GetID3D11DeviceContextClearRenderTargetViewRefCount()
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

extern "C" HRESULT WINAPI
  IDXGISwapChainPresentDetour(IDXGISwapChain* swap_chain,
                              UINT sync_interval,
                              UINT flags)
{
  DetourRefCounter ref_count{GetIDXGISwapChainPresentRefCount()};

  hadesmem::detail::LastErrorPreserver last_error;
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "IDXGISwapChainPresentDetour: Args: [%p] [%u] [%u].",
    swap_chain,
    sync_interval,
    flags);
  auto& detour = GetIDXGISwapChainPresentDetour();
  auto const present =
    detour->GetTrampoline<decltype(&IDXGISwapChainPresentDetour)>();

  static std::once_flag once;
  auto const init = [&]()
  {
    if (SUCCEEDED(swap_chain->GetDevice(__uuidof(g_device), reinterpret_cast<void**>(&g_device))))
    {
      g_device->GetImmediateContext(&g_device_context);

      // Put init code here.
    }
  };
  std::call_once(once, init);

  // Put drawing code here.

  last_error.Revert();
  auto const ret = present(swap_chain, sync_interval, flags);
  last_error.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("NtQuerySystemInformationDetour: Ret: [%ld].",
                                 ret);
  return ret;
}

extern "C" void WINAPI
  ID3D11DeviceContextDrawIndexedDetour(ID3D11DeviceContext* context,
                                       UINT index_count,
                                       UINT start_index_location,
                                       INT base_vertex_location)
{
  DetourRefCounter ref_count{GetID3D11DeviceContextDrawIndexedRefCount()};

  hadesmem::detail::LastErrorPreserver last_error;
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "ID3D11DeviceContextDrawIndexedDetour: Args: [%p] [%u] [%u] [%d].",
    context,
    index_count,
    start_index_location,
    base_vertex_location);
  auto& detour = GetID3D11DeviceContextDrawIndexedDetour();
  auto const draw_indexed =
    detour->GetTrampoline<decltype(&ID3D11DeviceContextDrawIndexedDetour)>();
  last_error.Revert();
  draw_indexed(
    context, index_count, start_index_location, base_vertex_location);
  last_error.Update();
  return;
}

extern "C" void WINAPI ID3D11DeviceContextClearRenderTargetViewDetour(
  ID3D11DeviceContext* context,
  ID3D11RenderTargetView* render_target_view,
  const FLOAT color_rgba[4])
{
  DetourRefCounter ref_count{
    GetID3D11DeviceContextClearRenderTargetViewRefCount()};

  hadesmem::detail::LastErrorPreserver last_error;
  HADESMEM_DETAIL_TRACE_FORMAT_A("ID3D11DeviceContextClearRenderTargetViewDetou"
                                 "r: Args: [%p] [%p] [%f] [%f] [%f] [%f].",
                                 context,
                                 render_target_view,
                                 color_rgba[0],
                                 color_rgba[1],
                                 color_rgba[2],
                                 color_rgba[3]);
  auto& detour = GetID3D11DeviceContextClearRenderTargetViewDetour();
  auto const clear_render_target_view = detour->GetTrampoline<
    decltype(&ID3D11DeviceContextClearRenderTargetViewDetour)>();
  last_error.Revert();
  clear_render_target_view(context, render_target_view, color_rgba);
  last_error.Update();
  return;
}

struct D3D11Funcs
{
  void* present;
  void* draw_indexed;
  void* clear_render_target_view;
};

D3D11Funcs FindD3D11Funcs()
{
  HWND const window = GetForegroundWindow();
  D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
  DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
  swap_chain_desc.BufferCount = 1;
  swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_chain_desc.OutputWindow = window;
  swap_chain_desc.SampleDesc.Count = 1;
  swap_chain_desc.Windowed =
    !!(GetWindowLong(window, GWL_STYLE) & WS_POPUP) ? FALSE : TRUE;
  swap_chain_desc.BufferDesc.ScanlineOrdering =
    DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  IDXGISwapChain* swap_chain = nullptr;
  ID3D11Device* device = nullptr;
  ID3D11DeviceContext* device_context = nullptr;
  HRESULT const hr = ::D3D11CreateDeviceAndSwapChain(nullptr,
                                                   D3D_DRIVER_TYPE_HARDWARE,
                                                   nullptr,
                                                   0,
                                                   &feature_level,
                                                   1,
                                                   D3D11_SDK_VERSION,
                                                   &swap_chain_desc,
                                                   &swap_chain,
                                                   &device,
                                                   nullptr,
                                                   &device_context);
  if (FAILED(hr))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{
                             "D3D11CreateDeviceAndSwapChain failed."}
                        << hadesmem::ErrorCodeWinHr{hr});
  }
  hadesmem::detail::SmartComHandle const swap_chain_cleanup(swap_chain);
  hadesmem::detail::SmartComHandle const device_cleanup(device);
  hadesmem::detail::SmartComHandle const device_context_cleanup(device_context);

  void** swap_chain_vtable = *reinterpret_cast<void***>(swap_chain);
  void** device_context_vtable = *reinterpret_cast<void***>(g_device_context);

  D3D11Funcs funcs{};
  funcs.present = swap_chain_vtable[8];
  funcs.draw_indexed = device_context_vtable[12];
  funcs.clear_render_target_view = device_context_vtable[50];

  return funcs;
}
}

void DetourD3D11()
{
  auto const funcs = FindD3D11Funcs();

  {
    auto const present_detour =
      hadesmem::detail::UnionCast<void*>(&IDXGISwapChainPresentDetour);
    auto& detour = GetIDXGISwapChainPresentDetour();
    detour = std::make_unique<hadesmem::PatchDetour>(
      GetThisProcess(), funcs.present, present_detour);
    detour->Apply();
    HADESMEM_DETAIL_TRACE_A("DetourD3D11: IDXGISwapChain::Present detoured.");
  }

  {
    auto const draw_indexed_detour =
      hadesmem::detail::UnionCast<void*>(&ID3D11DeviceContextDrawIndexedDetour);
    auto& detour = GetID3D11DeviceContextDrawIndexedDetour();
    detour = std::make_unique<hadesmem::PatchDetour>(
      GetThisProcess(), funcs.draw_indexed, draw_indexed_detour);
    detour->Apply();
    HADESMEM_DETAIL_TRACE_A(
      "DetourD3D11: ID3D11DeviceContext::DrawIndexed detoured.");
  }

  {
    auto const clear_render_target_view_detour =
      hadesmem::detail::UnionCast<void*>(
        &ID3D11DeviceContextClearRenderTargetViewDetour);
    auto& detour = GetID3D11DeviceContextClearRenderTargetViewDetour();
    detour =
      std::make_unique<hadesmem::PatchDetour>(GetThisProcess(),
                                              funcs.clear_render_target_view,
                                              clear_render_target_view_detour);
    detour->Apply();
    HADESMEM_DETAIL_TRACE_A(
      "DetourD3D11: ID3D11DeviceContext::ClearRenderTargetView detoured.");
  }
}

void UndetourD3D11()
{
  {
    auto& detour = GetIDXGISwapChainPresentDetour();
    detour->Remove();
    HADESMEM_DETAIL_TRACE_A(
      "UndetourD3D11: IDXGISwapChain::Present undetoured.");
    detour = nullptr;

    auto& ref_count = GetIDXGISwapChainPresentRefCount();
    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_A(
        "UndetourD3D11: Spinning on IDXGISwapChain::Present ref count.");
    }
    HADESMEM_DETAIL_TRACE_A(
      "UndetourD3D11: IDXGISwapChain::Present free of references.");
  }

  {
    auto& detour = GetID3D11DeviceContextDrawIndexedDetour();
    detour->Remove();
    HADESMEM_DETAIL_TRACE_A(
      "UndetourD3D11: ID3D11DeviceContext::DrawIndexed undetoured.");
    detour = nullptr;

    auto& ref_count = GetID3D11DeviceContextDrawIndexedRefCount();
    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_A("UndetourD3D11: Spinning on "
                              "ID3D11DeviceContext::DrawIndexed ref count.");
    }
    HADESMEM_DETAIL_TRACE_A(
      "UndetourD3D11: ID3D11DeviceContext::DrawIndexed free of references.");
  }

  {
    auto& detour = GetID3D11DeviceContextClearRenderTargetViewDetour();
    detour->Remove();
    HADESMEM_DETAIL_TRACE_A(
      "UndetourD3D11: ID3D11DeviceContext::ClearRenderTargetView undetoured.");
    detour = nullptr;

    auto& ref_count = GetID3D11DeviceContextClearRenderTargetViewRefCount();
    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_A("UndetourD3D11: Spinning on "
                              "ID3D11DeviceContext::ClearRenderTargetView ref "
                              "count.");
    }
    HADESMEM_DETAIL_TRACE_A("UndetourD3D11: "
                            "ID3D11DeviceContext::ClearRenderTargetView free "
                            "of references.");
  }
}
