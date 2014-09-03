// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "render.hpp"

#include <cstdint>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <d3d9.h>
#include <d3d11.h>
#include <d3d10.h>
#include <dxgi.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <anttweakbar.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/smart_handle.hpp>

#include "callbacks.hpp"
#include "d3d9.hpp"
#include "dxgi.hpp"
#include "main.hpp"

namespace
{

struct WindowInfo
{
  HWND old_hwnd_{nullptr};
  WNDPROC old_wndproc_{nullptr};
};

WindowInfo& GetWindowInfo() HADESMEM_DETAIL_NOEXCEPT
{
  static WindowInfo window_info;
  return window_info;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  LRESULT ret = 0;
  WindowInfo& window_info = GetWindowInfo();

  // Window #0 will always exist if TwInit has completed successfully.
  if (TwWindowExists(0) && TwEventWin(hwnd, msg, wparam, lparam))
  {
    // Event has been handled by AntTweakBar.
    ret = 0;
  }
  else if (window_info.old_wndproc_ != nullptr)
  {
    ret =
      ::CallWindowProcW(window_info.old_wndproc_, hwnd, msg, wparam, lparam);
  }
  else
  {
    ret = ::DefWindowProcW(hwnd, msg, wparam, lparam);
  }

  return ret;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallback>&
  GetOnFrameCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallback>
    callbacks;
  return callbacks;
}

struct RenderInfoDXGI
{
  bool first_time_{true};
  bool tw_initialized_{false};
  bool wndproc_swapped_{false};
  IDXGISwapChain* swap_chain_{};
};

struct RenderInfoD3D11 : RenderInfoDXGI
{
  ID3D11Device* device_;
};

RenderInfoD3D11& GetRenderInfoD3D11() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoD3D11 render_info;
  return render_info;
}

struct RenderInfoD3D10 : RenderInfoDXGI
{
  ID3D10Device* device_;
};

RenderInfoD3D10& GetRenderInfoD3D10() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoD3D10 render_info;
  return render_info;
}

struct RenderInfoD3D9
{
  bool first_time_{true};
  bool tw_initialized_{false};
  bool wndproc_swapped_{false};
  IDirect3DDevice9* device_{};
};

RenderInfoD3D9& GetRenderInfoD3D9() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoD3D9 render_info;
  return render_info;
}

bool AntTweakBarInitializedAny()
{
  return GetRenderInfoD3D9().tw_initialized_ ||
         GetRenderInfoD3D10().tw_initialized_ ||
         GetRenderInfoD3D11().tw_initialized_;
}

void InitializeWndprocHook(RenderInfoDXGI& render_info)
{
  WindowInfo& window_info = GetWindowInfo();
  if (window_info.old_wndproc_ == nullptr)
  {
    DXGI_SWAP_CHAIN_DESC desc;
    auto const get_desc_hr = render_info.swap_chain_->GetDesc(&desc);
    if (FAILED(get_desc_hr))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"GetDesc failed."}
                          << hadesmem::ErrorCodeWinHr{get_desc_hr});
    }
    window_info.old_hwnd_ = desc.OutputWindow;
    window_info.old_wndproc_ = reinterpret_cast<WNDPROC>(
      ::SetWindowLongPtrW(desc.OutputWindow,
                          GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(&WindowProc)));
    render_info.wndproc_swapped_ = true;
    HADESMEM_DETAIL_TRACE_FORMAT_A("Replaced window procedure located at %p.",
                                   window_info.old_wndproc_);
  }
}

void InitializeWndprocHook(RenderInfoD3D9& render_info)
{
  WindowInfo& window_info = GetWindowInfo();
  if (window_info.old_wndproc_ == nullptr)
  {
    IDirect3D9* d3d9 = nullptr;
    render_info.device_->GetDirect3D(&d3d9);
    D3DDEVICE_CREATION_PARAMETERS create_params;
    ::ZeroMemory(&create_params, sizeof(create_params));
    auto const get_create_params_hr =
      render_info.device_->GetCreationParameters(&create_params);
    if (FAILED(get_create_params_hr))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
                               "GetCreationParameters failed."}
                          << hadesmem::ErrorCodeWinHr{get_create_params_hr});
    }
    window_info.old_hwnd_ = create_params.hFocusWindow;
    window_info.old_wndproc_ = reinterpret_cast<WNDPROC>(
      ::SetWindowLongPtrW(create_params.hFocusWindow,
                          GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(&WindowProc)));
    render_info.wndproc_swapped_ = true;
    HADESMEM_DETAIL_TRACE_FORMAT_A("Replaced window procedure located at %p.",
                                   window_info.old_wndproc_);
  }
}

void InitializeAntTweakBar(TwGraphAPI api, void* device, bool& initialized)
{
  if (AntTweakBarInitializedAny())
  {
    HADESMEM_DETAIL_TRACE_A(
      "WARNING! AntTweakBar is already initialized. Skipping.");
    return;
  }

  if (!TwInit(api, device))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"TwInit failed."});
  }

  initialized = true;

  auto& window_info = GetWindowInfo();
  RECT wnd_rect{0, 0, 0, 0};
  if (!window_info.old_hwnd_ ||
      !GetWindowRect(window_info.old_hwnd_, &wnd_rect))
  {
    wnd_rect = RECT{0, 0, 800, 600};
  }

  if (!TwWindowSize(wnd_rect.right, wnd_rect.bottom))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwWindowSize failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  auto const bar = TwNewBar("HadesMem");
  if (!bar)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwNewBar failed."});
  }
}

void CleanupAntTweakBar(bool& initialized)
{
  if (initialized)
  {
    if (!TwTerminate())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"TwTerminate failed."});
    }

    initialized = false;
  }
}

void CleanupWndproc(bool& wndproc_swapped)
{
  if (wndproc_swapped)
  {
    WindowInfo& window_info = GetWindowInfo();
    if (window_info.old_wndproc_ != nullptr)
    {
      ::SetWindowLongPtrW(window_info.old_hwnd_,
                          GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(window_info.old_wndproc_));
      HADESMEM_DETAIL_TRACE_FORMAT_A("Reset window procedure located at %p.",
                                     window_info.old_wndproc_);
      window_info.old_hwnd_ = nullptr;
      window_info.old_wndproc_ = nullptr;
      wndproc_swapped = false;
    }
  }
}

void HandleChangedSwapChainD3D11(IDXGISwapChain* swap_chain,
                                 RenderInfoD3D11& render_info)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Got a new swap chain. Old = %p, New = %p.",
                                 render_info.swap_chain_,
                                 swap_chain);
  render_info.swap_chain_ = swap_chain;

  render_info.device_ = nullptr;

  render_info.first_time_ = true;

  CleanupAntTweakBar(render_info.tw_initialized_);

  CleanupWndproc(render_info.wndproc_swapped_);
}

void HandleChangedSwapChainD3D10(IDXGISwapChain* swap_chain,
                                 RenderInfoD3D10& render_info)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Got a new swap chain. Old = %p, New = %p.",
                                 render_info.swap_chain_,
                                 swap_chain);
  render_info.swap_chain_ = swap_chain;

  render_info.device_ = nullptr;

  render_info.first_time_ = true;

  CleanupAntTweakBar(render_info.tw_initialized_);

  CleanupWndproc(render_info.wndproc_swapped_);
}

void HandleChangedDeviceD3D9(IDirect3DDevice9* device,
                             RenderInfoD3D9& render_info)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Got a new device. Old = %p, New = %p.", render_info.device_, device);
  render_info.device_ = device;

  render_info.first_time_ = true;

  CleanupAntTweakBar(render_info.tw_initialized_);

  CleanupWndproc(render_info.wndproc_swapped_);
}

void InitializeD3D11RenderInfo(RenderInfoD3D11& render_info)
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  render_info.first_time_ = false;

  auto const get_device_hr = render_info.swap_chain_->GetDevice(
    __uuidof(render_info.device_),
    reinterpret_cast<void**>(&render_info.device_));
  if (FAILED(get_device_hr))
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "WARNING! IDXGISwapChain::GetDevice failed. HR = %08X.", get_device_hr);
    return;
  }

  InitializeWndprocHook(render_info);

  InitializeAntTweakBar(
    TW_DIRECT3D11, render_info.device_, render_info.tw_initialized_);

  HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
}

void InitializeD3D10RenderInfo(RenderInfoD3D10& render_info)
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  render_info.first_time_ = false;

  auto const get_device_hr = render_info.swap_chain_->GetDevice(
    __uuidof(render_info.device_),
    reinterpret_cast<void**>(&render_info.device_));
  if (FAILED(get_device_hr))
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "WARNING! IDXGISwapChain::GetDevice failed. HR = %08X.", get_device_hr);
    return;
  }

  InitializeWndprocHook(render_info);

  InitializeAntTweakBar(
    TW_DIRECT3D10, render_info.device_, render_info.tw_initialized_);

  HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
}

void InitializeD3D9RenderInfo(RenderInfoD3D9& render_info)
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  render_info.first_time_ = false;

  InitializeWndprocHook(render_info);

  InitializeAntTweakBar(
    TW_DIRECT3D9, render_info.device_, render_info.tw_initialized_);

  HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
}

void HandleOnFrameD3D11(IDXGISwapChain* swap_chain)
{
  auto& render_info = GetRenderInfoD3D11();
  if (render_info.swap_chain_ != swap_chain)
  {
    HandleChangedSwapChainD3D11(swap_chain, render_info);
  }

  if (render_info.first_time_)
  {
    InitializeD3D11RenderInfo(render_info);
  }
}

void HandleOnFrameD3D10(IDXGISwapChain* swap_chain)
{
  auto& render_info = GetRenderInfoD3D10();
  if (render_info.swap_chain_ != swap_chain)
  {
    HandleChangedSwapChainD3D10(swap_chain, render_info);
  }

  if (render_info.first_time_)
  {
    InitializeD3D10RenderInfo(render_info);
  }
}

void HandleOnFrameD3D9(IDirect3DDevice9* device)
{
  auto& render_info = GetRenderInfoD3D9();
  if (render_info.device_ != device)
  {
    HandleChangedDeviceD3D9(device, render_info);
  }

  if (render_info.first_time_)
  {
    InitializeD3D9RenderInfo(render_info);
  }
}

void OnFrameDXGI(IDXGISwapChain* swap_chain)
{
  HandleOnFrameD3D11(swap_chain);

  HandleOnFrameD3D10(swap_chain);

  auto& callbacks = GetOnFrameCallbacks();
  auto& render_interface = hadesmem::cerberus::GetRenderInterface();
  callbacks.Run(&render_interface);
}

void OnFrameD3D9(IDirect3DDevice9* device)
{
  HandleOnFrameD3D9(device);

  auto& callbacks = GetOnFrameCallbacks();
  auto& render_interface = hadesmem::cerberus::GetRenderInterface();
  callbacks.Run(&render_interface);
}

void DrawTweakBarImpl()
{
  if (!TwDraw())
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"TwDraw failed."});
  }
}

class RenderImpl : public hadesmem::cerberus::RenderInterface
{
public:
  virtual ~RenderImpl() final
  {
    bool initialized = AntTweakBarInitializedAny();
    CleanupAntTweakBar(initialized);
  }

  virtual std::size_t RegisterOnFrame(
    std::function<hadesmem::cerberus::OnFrameCallback> const& callback) final
  {
    return hadesmem::cerberus::RegisterOnFrameCallback(callback);
  }

  virtual void UnregisterOnFrame(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnFrameCallback(id);
  }

  virtual void DrawTweakBar() final
  {
    DrawTweakBarImpl();
  }
};
}

namespace hadesmem
{

namespace cerberus
{

RenderInterface& GetRenderInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderImpl render_impl;
  return render_impl;
}

void InitializeRender()
{
  auto const dxgi_callback_id =
    hadesmem::cerberus::RegisterOnFrameCallbackDXGI(OnFrameDXGI);
  (void)dxgi_callback_id;

  auto const d3d9_callback_id =
    hadesmem::cerberus::RegisterOnFrameCallbackD3D9(OnFrameD3D9);
  (void)d3d9_callback_id;

  auto const draw_tweak_bar = [](RenderInterface* render)
  { render->DrawTweakBar(); };
  RegisterOnFrameCallback(draw_tweak_bar);
}

std::size_t
  RegisterOnFrameCallback(std::function<OnFrameCallback> const& callback)
{
  auto& callbacks = GetOnFrameCallbacks();
  return callbacks.Register(callback);
}

void UnregisterOnFrameCallback(std::size_t id)
{
  auto& callbacks = GetOnFrameCallbacks();
  return callbacks.Unregister(id);
}
}
}
