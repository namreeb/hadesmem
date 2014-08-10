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

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallback>&
  GetOnFrameCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallback>
    callbacks;
  return callbacks;
}

struct RenderInfoD3D11
{
  bool first_time_{true};
  bool tw_initialized_{false};
  IDXGISwapChain* swap_chain_{};
  ID3D11Device* device_;
};

RenderInfoD3D11& GetRenderInfoD3D11() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoD3D11 render_info;
  return render_info;
}

struct RenderInfoD3D10
{
  bool first_time_{true};
  bool tw_initialized_{false};
  IDXGISwapChain* swap_chain_{};
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
  IDirect3DDevice9* device_{};
};

RenderInfoD3D9& GetRenderInfoD3D9() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoD3D9 render_info;
  return render_info;
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

  if (render_info.tw_initialized_)
  {
    if (!TwTerminate())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"TwTerminate failed."});
    }

    render_info.tw_initialized_ = false;
  }
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

  if (render_info.tw_initialized_)
  {
    if (!TwTerminate())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"TwTerminate failed."});
    }

    render_info.tw_initialized_ = false;
  }
}

void HandleChangedDeviceD3D9(IDirect3DDevice9* device,
                             RenderInfoD3D9& render_info)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Got a new device. Old = %p, New = %p.", render_info.device_, device);
  render_info.device_ = device;

  render_info.first_time_ = true;

  if (render_info.tw_initialized_)
  {
    if (!TwTerminate())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"TwTerminate failed."});
    }

    render_info.tw_initialized_ = false;
  }
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
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{
                                         "IDXGISwapChain::GetDevice failed."}
                                    << hadesmem::ErrorCodeWinHr{get_device_hr});
  }

  if (!TwInit(TW_DIRECT3D11, render_info.device_))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"TwInit failed."});
  }

  render_info.tw_initialized_ = true;

  if (!TwWindowSize(800, 600))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwWindowSize failed."});
  }

  auto const bar = TwNewBar("HadesMem");
  if (!bar)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwNewBar failed."});
  }

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
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{
                                         "IDXGISwapChain::GetDevice failed."}
                                    << hadesmem::ErrorCodeWinHr{get_device_hr});
  }

  if (!TwInit(TW_DIRECT3D10, render_info.device_))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"TwInit failed."});
  }

  render_info.tw_initialized_ = true;

  if (!TwWindowSize(800, 600))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwWindowSize failed."});
  }

  auto const bar = TwNewBar("HadesMem");
  if (!bar)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwNewBar failed."});
  }

  HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
}

void InitializeD3D9RenderInfo(RenderInfoD3D9& render_info)
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  render_info.first_time_ = false;

  if (!TwInit(TW_DIRECT3D9, render_info.device_))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"TwInit failed."});
  }

  render_info.tw_initialized_ = true;

  if (!TwWindowSize(800, 600))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwWindowSize failed."});
  }

  auto const bar = TwNewBar("HadesMem");
  if (!bar)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwNewBar failed."});
  }

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
