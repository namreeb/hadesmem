// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "render.hpp"

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <queue>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <d3d11.h>
#include <d3d10.h>
#include <d3d9.h>
#include <dxgi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/str_conv.hpp>

#include "callbacks.hpp"
#include "config.hpp"
#include "cursor.hpp"
#include "d3d11_state_block.hpp"
#include "d3d9.hpp"
#include "direct_input.hpp"
#include "dxgi.hpp"
#include "hook_disabler.hpp"
#include "input.hpp"
#include "main.hpp"
#include "opengl.hpp"
#include "plugin.hpp"
#include "process.hpp"
#include "window.hpp"

// TODO: Test against games like CoD: Ghosts which apparently have both a D3D10
// and D3D11 device at the same time (only on Nvidia GPUs?).

// TODO: Render something on startup like the Steam overlay does so we can
// confirm that rendering is working even that cases that input isn't.

// TODO: Ensure that everything works correctly when we set default GUI
// visibility on (currently it is off by default and toggled on).

// TODO: Hook the device window not the focus window?

// TODO: Replace AntTweakBar with CEGUI? Need something with proper multi-device
// support, a roadmap for DX12, etc. CEGUI is a pretty gigantic dependency (with
// many of its own dependencies), but it seems to be the only real contender.
// AntTweakBar is abandoned, and seemingly so is GWEN.
// At the very least we should replace AntTweakBar with GWEN for the time being,
// so we can rip out all the cross-thread input nonsense which is causing
// various input handling issues in some games. Need to confirm GWEN is
// thread-safe and not doing rendering from its input injection APIs.

// TODO: DirectDraw rendering support (e.g. Abe's Oddysee).

// TODO: Investigate Chromium Embedded Framework backed GUI.

// TODO: Add a forced-windowed option. (Belongs in CXExample?)
// http://bit.ly/1LMqsS8

// TODO: Add a generic crosshair. (Belongs in CXExample?)

// TODO: Add basic framework for drawing shapes, text, etc. for use in
// extensions.

// TODO: Suport using both GWEN and AntTweakBar at the same time, with different
// keys bound to them.

// TODO: Add proper multi-device, and multi-swapchain support. Sample:
// http://bit.ly/1Nd4jzT http://bit.ly/1VSmgLy http://bit.ly/1Lb13nV

// TODO: Add proper multi-window support. Sample:
// http://bit.ly/1Nd4jzT

// TODO: Ensure we work as well as possible in the case of multiple games which
// 'fight' over control of the mouse (because we save/restore state, we need to
// ensure we do it properly). http://bit.ly/1XkDFKc

// TODO: Fix Steam overlay for games we have injected Cerberus into. E.g.
// Oddworld New and Tasty?

// TODO: When testing game support, double check that we're detecting the
// resolution correctly for AntTweakBar and that GUI elements are minimized to
// the bottom left corner, as previously we sometimes had issues where we were
// detecting the resolution as smaller than it actually was.

// TODO: Fix all our leaks on game shutdown.

// TODO: Simplify and genericize rendering code, especially the stuff related to
// the window hooking etc.

// TODO: Rewrite this to utilize the assumption that there is only ever one
// active 'device', because all GUI libs make that assumption anyway and it
// would vastly simplifiy our code.

// TODO: Fix problem where D3D9 games crash when a device reset happens (e.g.
// pop a UAC window with WoW in borderless windowed).

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallback>&
  GetOnFrameCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallback>&
  GetOnFrame2Callbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnResizeCallback>&
  GetOnResizeCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnResizeCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnSetGuiVisibilityCallback>&
  GetOnSetGuiVisibilityCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnSetGuiVisibilityCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnInitializeGuiCallback>&
  GetOnInitializeGuiCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnInitializeGuiCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnCleanupGuiCallback>&
  GetOnCleanupGuiCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnCleanupGuiCallback>
    callbacks;
  return callbacks;
}

class RenderImpl : public hadesmem::cerberus::RenderInterface
{
public:
  virtual std::size_t RegisterOnFrame(
    std::function<hadesmem::cerberus::OnFrameCallback> const& callback) final
  {
    auto& callbacks = GetOnFrameCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnFrame(std::size_t id) final
  {
    auto& callbacks = GetOnFrameCallbacks();
    return callbacks.Unregister(id);
  }
  virtual std::size_t RegisterOnFrame2(
    std::function<hadesmem::cerberus::OnFrameCallback> const& callback) final
  {
    auto& callbacks = GetOnFrame2Callbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnFrame2(std::size_t id) final
  {
    auto& callbacks = GetOnFrame2Callbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnResize(
    std::function<hadesmem::cerberus::OnResizeCallback> const& callback) final
  {
    auto& callbacks = GetOnResizeCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnResize(std::size_t id) final
  {
    auto& callbacks = GetOnResizeCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnSetGuiVisibility(
    std::function<hadesmem::cerberus::OnSetGuiVisibilityCallback> const&
      callback) final
  {
    auto& callbacks = GetOnSetGuiVisibilityCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnSetGuiVisibility(std::size_t id) final
  {
    auto& callbacks = GetOnSetGuiVisibilityCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnInitializeGui(
    std::function<hadesmem::cerberus::OnInitializeGuiCallback> const& callback)
    final
  {
    auto& callbacks = GetOnInitializeGuiCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnInitializeGui(std::size_t id) final
  {
    auto& callbacks = GetOnInitializeGuiCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnCleanupGui(
    std::function<hadesmem::cerberus::OnCleanupGuiCallback> const& callback)
    final
  {
    auto& callbacks = GetOnCleanupGuiCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnCleanupGui(std::size_t id) final
  {
    auto& callbacks = GetOnCleanupGuiCallbacks();
    return callbacks.Unregister(id);
  }
};

struct RenderInfoCommon
{
  bool wnd_hooked_{false};
};

struct RenderInfoDXGI : RenderInfoCommon
{
  IDXGISwapChain* swap_chain_{};
};

struct RenderInfoD3D11 : RenderInfoDXGI
{
  ID3D11Device* device_{};
};

RenderInfoD3D11& GetRenderInfoD3D11() noexcept
{
  static RenderInfoD3D11 render_info;
  return render_info;
}

struct RenderInfoD3D10 : RenderInfoDXGI
{
  ID3D10Device* device_{};
};

RenderInfoD3D10& GetRenderInfoD3D10() noexcept
{
  static RenderInfoD3D10 render_info;
  return render_info;
}

struct RenderInfoD3D9 : RenderInfoCommon
{
  IDirect3DDevice9* device_{};
};

RenderInfoD3D9& GetRenderInfoD3D9() noexcept
{
  static RenderInfoD3D9 render_info;
  return render_info;
}

struct RenderInfoOpenGL32 : RenderInfoCommon
{
  HDC device_{};
};

RenderInfoOpenGL32& GetRenderInfoOpenGL32() noexcept
{
  static RenderInfoOpenGL32 render_info;
  return render_info;
}

bool InitializeWndprocHook(RenderInfoDXGI& render_info)
{
  if (hadesmem::cerberus::IsWindowHooked())
  {
    HADESMEM_DETAIL_TRACE_A("Window is already hooked. Skipping hook request.");
    return false;
  }

  DXGI_SWAP_CHAIN_DESC desc;
  auto const get_desc_hr = render_info.swap_chain_->GetDesc(&desc);
  if (FAILED(get_desc_hr))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"GetDesc failed."}
                                    << hadesmem::ErrorCodeWinHr{get_desc_hr});
  }

  if (desc.OutputWindow)
  {
    hadesmem::cerberus::HandleWindowChange(desc.OutputWindow);
    return true;
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Null swap chain output window. Ignoring.");
    return false;
  }
}

bool InitializeWndprocHook(RenderInfoD3D9& render_info)
{
  if (hadesmem::cerberus::IsWindowHooked())
  {
    HADESMEM_DETAIL_TRACE_A("Window is already hooked. Skipping hook request.");
    return false;
  }

  IDirect3D9* d3d9 = nullptr;
  render_info.device_->GetDirect3D(&d3d9);
  D3DDEVICE_CREATION_PARAMETERS create_params;
  ::ZeroMemory(&create_params, sizeof(create_params));
  auto const get_create_params_hr =
    render_info.device_->GetCreationParameters(&create_params);
  if (FAILED(get_create_params_hr))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{}
      << hadesmem::ErrorString{"GetCreationParameters failed."}
      << hadesmem::ErrorCodeWinHr{get_create_params_hr});
  }

  // Pretty sure we should be doing something with hDeviceWindow from the
  // presentation params as well, depending on whether or not the game is
  // windowed...
  if (create_params.hFocusWindow)
  {
    hadesmem::cerberus::HandleWindowChange(create_params.hFocusWindow);
    return true;
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Null device focus window. Ignoring.");
    return false;
  }
}

bool InitializeWndprocHook(RenderInfoOpenGL32& render_info)
{
  if (hadesmem::cerberus::IsWindowHooked())
  {
    HADESMEM_DETAIL_TRACE_A("Window is already hooked. Skipping hook request.");

    return false;
  }

  if (HWND wnd = ::WindowFromDC(render_info.device_))
  {
    hadesmem::cerberus::HandleWindowChange(wnd);
    return true;
  }
  else
  {
    DWORD const last_error = ::GetLastError();
    (void)last_error;
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "Failed to get window handle (%ld). Ignoring.", last_error);
    return false;
  }
}

void InitializeGui(hadesmem::cerberus::RenderApi api, void* device)
{
  HADESMEM_DETAIL_TRACE_A("Calling GUI initialization callbacks.");

  auto const& callbacks = GetOnInitializeGuiCallbacks();
  callbacks.Run(api, device);

  HADESMEM_DETAIL_TRACE_A("Setting GUI visibilty.");

  auto const visible = hadesmem::cerberus::GetGuiVisible();
  hadesmem::cerberus::SetGuiVisible(visible, visible);

  HADESMEM_DETAIL_TRACE_A("Finished.");
}

void CleanupGui(hadesmem::cerberus::RenderApi api)
{
  HADESMEM_DETAIL_TRACE_A("Calling GUI cleanup callbacks.");

  auto const& callbacks = GetOnCleanupGuiCallbacks();
  callbacks.Run(api);
}

std::pair<hadesmem::cerberus::RenderApi, void*>
  GetDeviceFromSwapChain(IDXGISwapChain* swap_chain)
{
  // TODO: DirectX 12 support.

  ID3D11Device* d3d11_device = nullptr;
  auto const get_device_d3d11_hr = swap_chain->GetDevice(
    __uuidof(ID3D11Device), reinterpret_cast<void**>(&d3d11_device));
  if (SUCCEEDED(get_device_d3d11_hr) &&
      d3d11_device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
  {
    return {hadesmem::cerberus::RenderApi::kD3D11, d3d11_device};
  }

  ID3D10Device* d3d10_device = nullptr;
  auto const get_device_d3d10_hr = swap_chain->GetDevice(
    __uuidof(ID3D10Device), reinterpret_cast<void**>(&d3d10_device));
  if (SUCCEEDED(get_device_d3d10_hr))
  {
    return {hadesmem::cerberus::RenderApi::kD3D10, d3d10_device};
  }

  if (d3d11_device)
  {
    return {hadesmem::cerberus::RenderApi::kD3D11, d3d11_device};
  }

  return {hadesmem::cerberus::RenderApi::kInvalidMaxValue, nullptr};
}

bool HandleOnFrameD3D11(IDXGISwapChain* swap_chain, ID3D11Device* device)
{
  auto& render_info = GetRenderInfoD3D11();
  if (render_info.swap_chain_ != swap_chain || render_info.device_ != device)
  {
    if (render_info.swap_chain_ != swap_chain)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Got a new swap chain. Old = %p, New = %p.",
        render_info.swap_chain_,
        swap_chain);
    }

    if (render_info.device_ != device)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Got a new device. Old = %p, New = %p.", render_info.device_, device);
    }

    HADESMEM_DETAIL_TRACE_A("Cleaning up.");

    CleanupGui(hadesmem::cerberus::RenderApi::kD3D11);

    if (render_info.wnd_hooked_)
    {
      HADESMEM_DETAIL_TRACE_A("Unhooking window.");
      hadesmem::cerberus::HandleWindowChange(nullptr);
      render_info.wnd_hooked_ = false;
    }

    HADESMEM_DETAIL_TRACE_A("Setting new swap chain and/or device.");

    render_info.swap_chain_ = swap_chain;
    render_info.device_ = device;

    HADESMEM_DETAIL_TRACE_A("Initializing wndproc hook.");

    render_info.wnd_hooked_ = InitializeWndprocHook(render_info);

    HADESMEM_DETAIL_TRACE_A("Initializing GUI.");

    InitializeGui(hadesmem::cerberus::RenderApi::kD3D11, render_info.device_);

    HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
  }

  return true;
}

bool HandleOnFrameD3D10(IDXGISwapChain* swap_chain, ID3D10Device* device)
{
  auto& render_info = GetRenderInfoD3D10();
  if (render_info.swap_chain_ != swap_chain || render_info.device_ != device)
  {
    if (render_info.swap_chain_ != swap_chain)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Got a new swap chain. Old = %p, New = %p.",
        render_info.swap_chain_,
        swap_chain);
    }

    if (render_info.device_ != device)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Got a new device. Old = %p, New = %p.", render_info.device_, device);
    }

    HADESMEM_DETAIL_TRACE_A("Cleaning up.");

    CleanupGui(hadesmem::cerberus::RenderApi::kD3D10);

    if (render_info.wnd_hooked_)
    {
      HADESMEM_DETAIL_TRACE_A("Unhooking window.");
      hadesmem::cerberus::HandleWindowChange(nullptr);
      render_info.wnd_hooked_ = false;
    }

    HADESMEM_DETAIL_TRACE_A("Setting new swap chain and/or device.");

    render_info.swap_chain_ = swap_chain;
    render_info.device_ = device;

    HADESMEM_DETAIL_TRACE_A("Initializing wndproc hook.");

    render_info.wnd_hooked_ = InitializeWndprocHook(render_info);

    HADESMEM_DETAIL_TRACE_A("Initializing GUI.");

    InitializeGui(hadesmem::cerberus::RenderApi::kD3D10, render_info.device_);

    HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
  }

  return true;
}

void HandleOnFrameD3D9(IDirect3DDevice9* device)
{
  auto& render_info = GetRenderInfoD3D9();
  if (render_info.device_ != device)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "Got a new device. Old = %p, New = %p.", render_info.device_, device);

    render_info.device_ = device;

    HADESMEM_DETAIL_TRACE_A("Cleaning up.");

    CleanupGui(hadesmem::cerberus::RenderApi::kD3D9);

    if (render_info.wnd_hooked_)
    {
      HADESMEM_DETAIL_TRACE_A("Unhooking window.");
      hadesmem::cerberus::HandleWindowChange(nullptr);
      render_info.wnd_hooked_ = false;
    }

    HADESMEM_DETAIL_TRACE_A("Initializing wndproc hook.");

    render_info.wnd_hooked_ = InitializeWndprocHook(render_info);

    HADESMEM_DETAIL_TRACE_A("Initializing GUI.");

    InitializeGui(hadesmem::cerberus::RenderApi::kD3D9, render_info.device_);

    HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
  }
}

void HandleOnFrameOpenGL32(HDC device)
{
  auto& render_info = GetRenderInfoOpenGL32();
  if (render_info.device_ != device)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "Got a new device. Old = %p, New = %p.", render_info.device_, device);

    render_info.device_ = device;

    HADESMEM_DETAIL_TRACE_A("Cleaning up.");

    CleanupGui(hadesmem::cerberus::RenderApi::kOpenGL32);

    if (render_info.wnd_hooked_)
    {
      HADESMEM_DETAIL_TRACE_A("Unhooking window.");
      hadesmem::cerberus::HandleWindowChange(nullptr);
      render_info.wnd_hooked_ = false;
    }

    HADESMEM_DETAIL_TRACE_A("Initializing wndproc hook.");

    render_info.wnd_hooked_ = InitializeWndprocHook(render_info);

    HADESMEM_DETAIL_TRACE_A("Initializing GUI.");

    InitializeGui(hadesmem::cerberus::RenderApi::kOpenGL32,
                  render_info.device_);

    HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
  }
}

void OnFrameGeneric(hadesmem::cerberus::RenderApi api, void* device)
{
  HADESMEM_DETAIL_TRACE_NOISY_A("Calling OnFrame callbacks.");

  auto& callbacks = GetOnFrameCallbacks();
  callbacks.Run(api, device);

  HADESMEM_DETAIL_TRACE_NOISY_A("Calling OnFrame2 callbacks.");

  auto& callbacks2 = GetOnFrame2Callbacks();
  callbacks2.Run(api, device);

  HADESMEM_DETAIL_TRACE_NOISY_A("Calling HandleInputQueue.");

  hadesmem::cerberus::HandleInputQueue();

  HADESMEM_DETAIL_TRACE_NOISY_A("Done.");
}

void OnResizeGeneric(hadesmem::cerberus::RenderApi api,
                     void* device,
                     UINT width,
                     UINT height)
{
  auto& callbacks = GetOnResizeCallbacks();
  callbacks.Run(api, device, width, height);
}

// Stack usage intentional. D3D11StateBlock is large but we don't want to cause
// an easily avoidable heap allocation every frame.
#pragma warning(suppress : 6262)
void OnFrameDXGI(IDXGISwapChain* swap_chain)
{
  auto const typed_device = GetDeviceFromSwapChain(swap_chain);
  if (typed_device.first == hadesmem::cerberus::RenderApi::kD3D11)
  {
    auto const device = static_cast<ID3D11Device*>(typed_device.second);

    ID3D11DeviceContext* device_context = nullptr;
    device->GetImmediateContext(&device_context);

    hadesmem::cerberus::D3D11StateBlock state_block{device_context};

    HandleOnFrameD3D11(swap_chain, device);

    device_context->CSSetShader(nullptr, nullptr, 0);
    device_context->DSSetShader(nullptr, nullptr, 0);
    device_context->GSSetShader(nullptr, nullptr, 0);
    device_context->HSSetShader(nullptr, nullptr, 0);

    hadesmem::detail::SmartComHandle back_buffer_cleanup;
    hadesmem::detail::SmartComHandle rtv_cleanup;

    ID3D11Texture2D* back_buffer = nullptr;
    HRESULT hr = swap_chain->GetBuffer(
      0, __uuidof(*back_buffer), reinterpret_cast<void**>(&back_buffer));
    if (SUCCEEDED(hr))
    {
      back_buffer_cleanup = back_buffer;

      D3D11_TEXTURE2D_DESC bb_surf_desc = {};
      back_buffer->GetDesc(&bb_surf_desc);

      D3D11_VIEWPORT viewport = {};
      viewport.Width = static_cast<float>(bb_surf_desc.Width);
      viewport.Height = static_cast<float>(bb_surf_desc.Height);
      viewport.MaxDepth = 1;
      device_context->RSSetViewports(1, &viewport);

      ID3D11RenderTargetView* rtv = nullptr;
      hr = device->CreateRenderTargetView(back_buffer, nullptr, &rtv);
      if (SUCCEEDED(hr))
      {
        rtv_cleanup = rtv;
        device_context->OMSetRenderTargets(1, &rtv, nullptr);
      }
      else
      {
        HADESMEM_DETAIL_TRACE_FORMAT_A(
          "WARNING! ID3D11Device::CreateRenderTargetView failed. HR: [%ld].",
          hr);
      }
    }
    else
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "WARNING! IDXGISwapChain::GetBuffer failed. HR: [%ld].", hr);
    }

    D3D11_BLEND_DESC blend = {};
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    // TODO: Can we make this more efficient by only re-creating the blend state
    // when the device changes? What about the other objects we create like
    // render target views?
    hadesmem::detail::SmartComHandle blend_state_cleanup;
    ID3D11BlendState* blend_state = nullptr;
    hr = device->CreateBlendState(&blend, &blend_state);
    if (SUCCEEDED(hr))
    {
      // TODO: Double check that this change hasn't broken anything.
      blend_state_cleanup = blend_state;
      device_context->OMSetBlendState(blend_state, nullptr, 0xFFFFFFFF);
    }
    else
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "WARNING! ID3D11Device::CreateBlendState failed. HR: [%ld].", hr);
    }

    OnFrameGeneric(typed_device.first, typed_device.second);

    state_block.Apply();
  }
  else if (typed_device.first == hadesmem::cerberus::RenderApi::kD3D10)
  {
    auto const device = static_cast<ID3D10Device*>(typed_device.second);

    // TODO: Implement state block.
    // hadesmem::cerberus::D3D10StateBlock state_block{ device };

    HandleOnFrameD3D10(swap_chain, device);

    device->PSSetShader(nullptr);
    device->VSSetShader(nullptr);
    device->GSSetShader(nullptr);

    hadesmem::detail::SmartComHandle back_buffer_cleanup;
    hadesmem::detail::SmartComHandle rtv_cleanup;

    ID3D10Texture2D* back_buffer = nullptr;
    HRESULT hr = swap_chain->GetBuffer(
      0, __uuidof(*back_buffer), reinterpret_cast<void**>(&back_buffer));
    if (SUCCEEDED(hr))
    {
      back_buffer_cleanup = back_buffer;

      D3D10_TEXTURE2D_DESC bb_surf_desc = {};
      back_buffer->GetDesc(&bb_surf_desc);

      D3D10_VIEWPORT viewport = {};
      viewport.Width = bb_surf_desc.Width;
      viewport.Height = bb_surf_desc.Height;
      viewport.MaxDepth = 1;
      device->RSSetViewports(1, &viewport);

      ID3D10RenderTargetView* rtv = nullptr;
      hr = device->CreateRenderTargetView(back_buffer, nullptr, &rtv);
      if (SUCCEEDED(hr))
      {
        rtv_cleanup = rtv;
        device->OMSetRenderTargets(1, &rtv, nullptr);
      }
      else
      {
        HADESMEM_DETAIL_TRACE_FORMAT_A(
          "WARNING! ID3D10Device::CreateRenderTargetView failed. HR: [%ld].",
          hr);
      }
    }
    else
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "WARNING! IDXGISwapChain::GetBuffer failed. HR: [%ld].", hr);
    }

    D3D10_BLEND_DESC blend = {};
    blend.BlendEnable[0] = TRUE;
    blend.SrcBlend = D3D10_BLEND_ONE;
    blend.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
    blend.BlendOp = D3D10_BLEND_OP_ADD;
    blend.SrcBlendAlpha = D3D10_BLEND_ONE;
    blend.DestBlendAlpha = D3D10_BLEND_INV_SRC_ALPHA;
    blend.BlendOpAlpha = D3D10_BLEND_OP_ADD;
    blend.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

    // TODO: Can we make this more efficient by only re-creating the blend state
    // when the device changes? What about the other objects we create like
    // render target views?
    hadesmem::detail::SmartComHandle blend_state_cleanup;
    ID3D10BlendState* blend_state = nullptr;
    hr = device->CreateBlendState(&blend, &blend_state);
    if (SUCCEEDED(hr))
    {
      // TODO: Double check that this change hasn't broken anything.
      blend_state_cleanup = blend_state;
      device->OMSetBlendState(blend_state, nullptr, 0xFFFFFFFF);
    }
    else
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "WARNING! ID3D10Device::CreateBlendState failed. HR: [%ld].", hr);
    }

    OnFrameGeneric(typed_device.first, typed_device.second);

    // state_block.Apply();
  }
  else
  {
    // TODO: Trove is causing this. Figure out why.
    // HADESMEM_DETAIL_ASSERT(false);
    // HADESMEM_DETAIL_THROW_EXCEPTION(
    //  hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
    HADESMEM_DETAIL_TRACE_A("Unknown render API.");
  }
}

void OnResizeDXGI(IDXGISwapChain* swap_chain, UINT width, UINT height)
{
  // TODO: Is this correct? Do we also need to invalidate our swap chain/device
  // and re-initialize the GUIs?
  auto const typed_device = GetDeviceFromSwapChain(swap_chain);
  OnResizeGeneric(typed_device.first, typed_device.second, width, height);
}

// TODO: Do something similar for DX11.
void SetDefaultRenderStateD3D9(IDirect3DDevice9* device)
{
  device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
  device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
  device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
  device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
  device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
  device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
  device->SetRenderState(D3DRS_LASTPIXEL, TRUE);
  device->SetRenderState(D3DRS_FOGENABLE, FALSE);
  device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
  device->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0000000F);
  device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
  device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
  device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);

  device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
  device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
  device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
  device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU);
  device->SetTextureStageState(
    0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

  device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  device->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
  device->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0);
  device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
  device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
  device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
  device->SetSamplerState(0, D3DSAMP_MIPMAPLODBIAS, 0);
  device->SetSamplerState(0, D3DSAMP_MAXMIPLEVEL, 0);
  device->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);
  device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);
  device->SetSamplerState(0, D3DSAMP_ELEMENTINDEX, 0);
  device->SetSamplerState(0, D3DSAMP_DMAPOFFSET, 0);

  device->SetVertexShader(nullptr);
  device->SetPixelShader(nullptr);

  auto& window = hadesmem::cerberus::GetWindowInterface();
  auto const hwnd = window.GetCurrentWindow();

  RECT rect = {};
  ::GetClientRect(hwnd, &rect);

  D3DVIEWPORT9 vp = {};
  vp.Width = rect.right - rect.left;
  vp.Height = rect.bottom - rect.top;
  vp.MaxZ = 1;
  device->SetViewport(&vp);
}

void OnFrameD3D9(IDirect3DDevice9* device)
{
  auto const coop_level = device->TestCooperativeLevel();
  if (FAILED(coop_level))
  {
    HADESMEM_DETAIL_TRACE_A(
      "TestCooperativeLevel failed. Skipping rendering for this frame.");
    return;
  }

  HADESMEM_DETAIL_TRACE_NOISY_A("Calling IDirect3DDevice9::CreateStateBlock.");

  IDirect3DStateBlock9* state_block = nullptr;
  if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state_block)))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{
        "IDirect3DDevice9::CreateStateBlock failed."});
  }
  hadesmem::detail::SmartComHandle state_block_cleanup{state_block};

  HADESMEM_DETAIL_TRACE_NOISY_A("Calling HandleOnFrameD3D9.");

  HandleOnFrameD3D9(device);

  HADESMEM_DETAIL_TRACE_NOISY_A("Calling SetDefaultRenderStateD3D9.");

  SetDefaultRenderStateD3D9(device);

  HADESMEM_DETAIL_TRACE_NOISY_A("Calling OnFrameGeneric.");

  OnFrameGeneric(hadesmem::cerberus::RenderApi::kD3D9, device);

  HADESMEM_DETAIL_TRACE_NOISY_A("Calling IDirect3DStateBlock9::Apply.");

  if (FAILED(state_block->Apply()))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "IDirect3DStateBlock9::Apply failed."});
  }

  HADESMEM_DETAIL_TRACE_NOISY_A("Done.");
}

void OnFrameOpenGL32(HDC device)
{
  HandleOnFrameOpenGL32(device);

  OnFrameGeneric(hadesmem::cerberus::RenderApi::kOpenGL32, device);
}

// TODO: Actually handle pre-reset and post-reset properly. Right now we're
// getting called pre-reset and relying on an OnFrame event to reinitalize us.
void OnResetD3D9(IDirect3DDevice9* device,
                 D3DPRESENT_PARAMETERS* /*presentation_parameters*/)
{
  auto& render_info = GetRenderInfoD3D9();
  if (device == render_info.device_)
  {
    HADESMEM_DETAIL_TRACE_A("Handling D3D9 device reset.");

    HADESMEM_DETAIL_TRACE_A("Cleaning up.");

    CleanupGui(hadesmem::cerberus::RenderApi::kD3D9);

    if (render_info.wnd_hooked_)
    {
      HADESMEM_DETAIL_TRACE_A("Unhooking window.");
      hadesmem::cerberus::HandleWindowChange(nullptr);
      render_info.wnd_hooked_ = false;
    }

    HADESMEM_DETAIL_TRACE_A("Clearing current device.");

    render_info.device_ = nullptr;
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("WARNING! Detected release on unhandled "
                                   "device. Ours: [%p]. Theirs: [%p].",
                                   render_info.device_,
                                   device);
  }
}

void OnReleaseD3D9(IDirect3DDevice9* device)
{
  auto& render_info = GetRenderInfoD3D9();
  if (device == render_info.device_)
  {
    HADESMEM_DETAIL_TRACE_A("Calling CleanupGui.");

    CleanupGui(hadesmem::cerberus::RenderApi::kD3D9);

    if (render_info.wnd_hooked_)
    {
      HADESMEM_DETAIL_TRACE_A("Unhooking window.");
      hadesmem::cerberus::HandleWindowChange(nullptr);
      render_info.wnd_hooked_ = false;
    }

    HADESMEM_DETAIL_TRACE_A("Clearing device pointer.");

    render_info.device_ = nullptr;

    HADESMEM_DETAIL_TRACE_A("Finished handling D3D9 device release.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("WARNING! Detected release on unhandled "
                                   "device. Ours: [%p]. Theirs: [%p].",
                                   render_info.device_,
                                   device);
  }
}

void OnReleaseDXGI(IDXGISwapChain* swap_chain)
{
  bool cleaned = false;

  auto& render_info_d3d10 = GetRenderInfoD3D10();
  if (swap_chain == render_info_d3d10.swap_chain_)
  {
    HADESMEM_DETAIL_TRACE_A("Handling D3D10 device release.");

    CleanupGui(hadesmem::cerberus::RenderApi::kD3D10);

    cleaned = true;
  }

  auto& render_info_d3d11 = GetRenderInfoD3D11();
  if (swap_chain == render_info_d3d11.swap_chain_)
  {
    HADESMEM_DETAIL_TRACE_A("Handling D3D10 device release.");

    CleanupGui(hadesmem::cerberus::RenderApi::kD3D11);

    cleaned = true;
  }

  if (!cleaned)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "WARNING! Detected release on unhandled "
      "device. Ours (D3D10): [%p]. Ours (D3D11): [%p]. Theirs: [%p].",
      render_info_d3d10.swap_chain_,
      render_info_d3d11.swap_chain_,
      swap_chain);
  }
}

void OnRtlExitUserProcess(NTSTATUS /*exit_code*/)
{
  hadesmem::cerberus::CleanupRender();
}
}

namespace hadesmem
{
namespace cerberus
{
RenderInterface& GetRenderInterface() noexcept
{
  static RenderImpl render_impl;
  return render_impl;
}

std::string GetRenderApiName(hadesmem::cerberus::RenderApi api)
{
  switch (api)
  {
  case hadesmem::cerberus::RenderApi::kD3D9:
    return "D3D9";
  case hadesmem::cerberus::RenderApi::kD3D10:
    return "D3D10";
  case hadesmem::cerberus::RenderApi::kD3D11:
    return "D3D11";
  case hadesmem::cerberus::RenderApi::kOpenGL32:
    return "OpenGL";
  default:
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
  }
}

bool& GetGuiVisible() noexcept
{
  static bool visible{false};
  return visible;
}

void SetGuiVisible(bool visible, bool old_visible)
{
  HADESMEM_DETAIL_TRACE_A("Setting GUI visibility flag.");

  GetGuiVisible() = visible;

  HADESMEM_DETAIL_TRACE_A("Calling GUI visibility callbacks.");

  auto& callbacks = GetOnSetGuiVisibilityCallbacks();
  callbacks.Run(visible, old_visible);

  HADESMEM_DETAIL_TRACE_A("Performing input related GUI visibility tasks.");

  SetGuiVisibleForInput(visible, old_visible);

  HADESMEM_DETAIL_TRACE_A("Finished.");
}

void CleanupRender()
{
  hadesmem::cerberus::RenderApi const apis[] = {
    hadesmem::cerberus::RenderApi::kD3D9,
    hadesmem::cerberus::RenderApi::kD3D10,
    hadesmem::cerberus::RenderApi::kD3D11,
    hadesmem::cerberus::RenderApi::kOpenGL32};
  for (auto const& api : apis)
  {
    CleanupGui(api);
  }
}

void InitializeRender()
{
  auto& dxgi = GetDXGIInterface();
  dxgi.RegisterOnFrame(OnFrameDXGI);
  dxgi.RegisterOnResize(OnResizeDXGI);
  dxgi.RegisterOnRelease(OnReleaseDXGI);

  auto& d3d9 = GetD3D9Interface();
  d3d9.RegisterOnFrame(OnFrameD3D9);
  d3d9.RegisterOnReset(OnResetD3D9);
  d3d9.RegisterOnRelease(OnReleaseD3D9);

  // TODO: Re-enable this once we have a proper fix for cleaning up during
  // process shutdown in the case where the DX libs or other dependencies have
  // already been unloaded before ExitProcess is called. E.g. CSGO.
  // auto& process = GetProcessInterface();
  // process.RegisterOnRtlExitUserProcess(OnRtlExitUserProcess);

  auto& opengl32 = GetOpenGL32Interface();
  opengl32.RegisterOnFrame(OnFrameOpenGL32);
}
}
}
