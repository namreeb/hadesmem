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
#include <fw1fontwrapper.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>

#include "callbacks.hpp"
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
  IDXGISwapChain* swap_chain_{};
  ID3D11Device* device_;
  ID3D11DeviceContext* context_;
  IFW1Factory* fw1_factory_{};
  IFW1FontWrapper* font_wrapper_{};
};

RenderInfoD3D11& GetRenderInfoD3D11() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoD3D11 render_info;
  return render_info;
}

// Modified from http://bit.ly/1iizOJR
void HandleOnFrameD3D11(IDXGISwapChain* swap_chain)
{
  auto& render_info = GetRenderInfoD3D11();
  if (render_info.swap_chain_ != swap_chain)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("Got a new swap chain. Old = %p, New = %p.",
                                   render_info.swap_chain_,
                                   swap_chain);
    render_info.swap_chain_ = swap_chain;

    if (render_info.font_wrapper_)
    {
      render_info.font_wrapper_->Release();
      render_info.font_wrapper_ = nullptr;
    }

    render_info.device_ = nullptr;
    render_info.context_ = nullptr;

    render_info.first_time_ = true;
  }

  if (render_info.first_time_)
  {
    HADESMEM_DETAIL_TRACE_A("Initializing.");

    render_info.first_time_ = false;

    auto const get_device_hr =
      swap_chain->GetDevice(__uuidof(render_info.device_),
                            reinterpret_cast<void**>(&render_info.device_));
    if (FAILED(get_device_hr))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
                               "IDXGISwapChain::GetDevice failed."}
                          << hadesmem::ErrorCodeWinHr{get_device_hr});
    }

    render_info.device_->GetImmediateContext(&render_info.context_);

    auto const create_factory_hr =
      FW1CreateFactory(FW1_VERSION, &render_info.fw1_factory_);
    if (FAILED(create_factory_hr))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"FW1CreateFactory failed."}
                          << hadesmem::ErrorCodeWinHr{create_factory_hr});
    }

    auto const create_font_hr = render_info.fw1_factory_->CreateFontWrapper(
      render_info.device_, L"Consolas", &render_info.font_wrapper_);
    if (FAILED(create_font_hr))
    {
      render_info.fw1_factory_->Release();
      render_info.fw1_factory_ = nullptr;

      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
                               "IFW1Factory::CreateFontWrapper failed."}
                          << hadesmem::ErrorCodeWinHr{create_font_hr});
    }

    render_info.fw1_factory_->Release();
    render_info.fw1_factory_ = nullptr;
  }
}

void OnFrameDXGI(IDXGISwapChain* swap_chain)
{
  HandleOnFrameD3D11(swap_chain);

  auto& callbacks = GetOnFrameCallbacks();
  auto& render_interface = hadesmem::cerberus::GetRenderInterface();
  callbacks.Run(&render_interface);
}

void DrawWatermarkImpl()
{
  auto& render_info = GetRenderInfoD3D11();
  if (render_info.font_wrapper_)
  {
    render_info.font_wrapper_->DrawString(render_info.context_,
                                          L"HadesMem (D3D11 Mode)",
                                          15.0f,
                                          15.0f,
                                          15.0f,
                                          0xFF2525FF,
                                          FW1_RESTORESTATE);
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

  virtual void DrawWatermark() final
  {
    DrawWatermarkImpl();
  }
};
}

namespace hadesmem
{

namespace cerberus
{

void InitializeRender()
{
  auto const dxgi_callback_id =
    hadesmem::cerberus::RegisterOnFrameCallbackDXGI(OnFrameDXGI);
  (void)dxgi_callback_id;

  auto const draw_watermark = [](RenderInterface* render)
  { render->DrawWatermark(); };
  RegisterOnFrameCallback(draw_watermark);
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

hadesmem::cerberus::RenderInterface& GetRenderInterface()
  HADESMEM_DETAIL_NOEXCEPT
{
  static RenderImpl render_impl;
  return render_impl;
}
}
}
