// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "render.hpp"

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <queue>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <d3d9.h>
#include <d3d11.h>
#include <d3d10.h>
#include <dxgi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/str_conv.hpp>

#include "callbacks.hpp"
#include "cursor.hpp"
#include "d3d9.hpp"
#include "d3d10.hpp"
#include "d3d11.hpp"
#include "d3d11_state_block.hpp"
#include "direct_input.hpp"
#include "dxgi.hpp"
#include "hook_disabler.hpp"
#include "input.hpp"
#include "opengl.hpp"
#include "plugin.hpp"
#include "main.hpp"
#include "window.hpp"

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallback>&
  GetOnFrameCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallback>
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

  virtual std::size_t RegisterOnSetGuiVisibility(std::function<
    hadesmem::cerberus::OnSetGuiVisibilityCallback> const& callback) final
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

RenderInfoD3D11& GetRenderInfoD3D11() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoD3D11 render_info;
  return render_info;
}

struct RenderInfoD3D10 : RenderInfoDXGI
{
  ID3D10Device* device_{};
};

RenderInfoD3D10& GetRenderInfoD3D10() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoD3D10 render_info;
  return render_info;
}

struct RenderInfoD3D9 : RenderInfoCommon
{
  IDirect3DDevice9* device_{};
};

RenderInfoD3D9& GetRenderInfoD3D9() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoD3D9 render_info;
  return render_info;
}

struct RenderInfoOpenGL32 : RenderInfoCommon
{
  HDC device_{};
};

RenderInfoOpenGL32& GetRenderInfoOpenGL32() HADESMEM_DETAIL_NOEXCEPT
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
      hadesmem::Error{} << hadesmem::ErrorString{
                             "GetCreationParameters failed."}
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
  ID3D11Device* d3d11_device = nullptr;
  auto const get_device_d3d11_hr = swap_chain->GetDevice(
    __uuidof(ID3D11Device), reinterpret_cast<void**>(&d3d11_device));
  if (SUCCEEDED(get_device_d3d11_hr))
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

  return {hadesmem::cerberus::RenderApi::kInvalidMaxValue, nullptr};
}

bool HandleOnFrameD3D11(IDXGISwapChain* swap_chain)
{
  auto& render_info = GetRenderInfoD3D11();
  if (render_info.swap_chain_ != swap_chain)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("Got a new swap chain. Old = %p, New = %p.",
                                   render_info.swap_chain_,
                                   swap_chain);

    render_info.swap_chain_ = swap_chain;

    HADESMEM_DETAIL_TRACE_A("Cleaning up.");

    render_info.device_ = nullptr;

    CleanupGui(hadesmem::cerberus::RenderApi::kD3D11);

    if (render_info.wnd_hooked_)
    {
      HADESMEM_DETAIL_TRACE_A("Unhooking window.");
      hadesmem::cerberus::HandleWindowChange(nullptr);
      render_info.wnd_hooked_ = false;
    }

    HADESMEM_DETAIL_TRACE_A("Getting device.");

    auto const get_device_hr = render_info.swap_chain_->GetDevice(
      __uuidof(ID3D11Device), reinterpret_cast<void**>(&render_info.device_));
    if (FAILED(get_device_hr))
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "WARNING! IDXGISwapChain::GetDevice failed. HR = %08X.", get_device_hr);
      return false;
    }

    HADESMEM_DETAIL_TRACE_A("Initializing wndproc hook.");

    render_info.wnd_hooked_ = InitializeWndprocHook(render_info);

    HADESMEM_DETAIL_TRACE_A("Initializing GUI.");

    InitializeGui(hadesmem::cerberus::RenderApi::kD3D11, render_info.device_);

    HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
  }

  return true;
}

bool HandleOnFrameD3D10(IDXGISwapChain* swap_chain)
{
  auto& render_info = GetRenderInfoD3D10();
  if (render_info.swap_chain_ != swap_chain)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("Got a new swap chain. Old = %p, New = %p.",
                                   render_info.swap_chain_,
                                   swap_chain);

    render_info.swap_chain_ = swap_chain;

    HADESMEM_DETAIL_TRACE_A("Cleaning up.");

    render_info.device_ = nullptr;

    CleanupGui(hadesmem::cerberus::RenderApi::kD3D10);

    if (render_info.wnd_hooked_)
    {
      HADESMEM_DETAIL_TRACE_A("Unhooking window.");
      hadesmem::cerberus::HandleWindowChange(nullptr);
      render_info.wnd_hooked_ = false;
    }

    HADESMEM_DETAIL_TRACE_A("Getting device.");

    auto const get_device_hr = render_info.swap_chain_->GetDevice(
      __uuidof(ID3D10Device), reinterpret_cast<void**>(&render_info.device_));
    if (FAILED(get_device_hr))
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "WARNING! IDXGISwapChain::GetDevice failed. HR = %08X.", get_device_hr);
      return false;
    }

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

void HandleOnResetD3D9(IDirect3DDevice9* device,
                       D3DPRESENT_PARAMETERS* /*presentation_parameters*/)
{
  auto& render_info = GetRenderInfoD3D9();
  if (device == render_info.device_)
  {
    HADESMEM_DETAIL_TRACE_A("Handling D3D9 device reset.");

    CleanupGui(hadesmem::cerberus::RenderApi::kD3D9);

    InitializeGui(hadesmem::cerberus::RenderApi::kD3D9, render_info.device_);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "WARNING! Detected reset on unknown device. Ours = %p, Theirs = %p.",
      render_info.device_,
      device);
  }
}

void OnFrameGeneric(hadesmem::cerberus::RenderApi api, void* device)
{
  auto& callbacks = GetOnFrameCallbacks();
  callbacks.Run(api, device);

  hadesmem::cerberus::HandleInputQueue();
}

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 6262)
#endif // #if defined(HADESMEM_MSVC)

void OnFrameDXGI(IDXGISwapChain* swap_chain)
{
  HandleOnFrameD3D11(swap_chain);
  HandleOnFrameD3D10(swap_chain);

  auto const typed_device = GetDeviceFromSwapChain(swap_chain);
  if (typed_device.first == hadesmem::cerberus::RenderApi::kD3D11)
  {
    auto const device = static_cast<ID3D11Device*>(typed_device.second);
    ID3D11DeviceContext* device_context = nullptr;
    device->GetImmediateContext(&device_context);
    hadesmem::cerberus::D3D11StateBlock state_block{device_context};

    device_context->CSSetShader(nullptr, nullptr, 0);
    device_context->DSSetShader(nullptr, nullptr, 0);
    device_context->GSSetShader(nullptr, nullptr, 0);
    device_context->HSSetShader(nullptr, nullptr, 0);

    OnFrameGeneric(typed_device.first, typed_device.second);

    state_block.Apply();
  }
  else if (typed_device.first == hadesmem::cerberus::RenderApi::kD3D10)
  {
    OnFrameGeneric(typed_device.first, typed_device.second);
  }
  else
  {
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
  }
}

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

void OnFrameD3D9(IDirect3DDevice9* device)
{
  HandleOnFrameD3D9(device);

  IDirect3DStateBlock9* state_block = nullptr;
  if (FAILED(device->CreateStateBlock(D3DSBT_ALL, &state_block)))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{
        "IDirect3DDevice9::CreateStateBlock failed."});
  }
  hadesmem::detail::SmartComHandle state_block_cleanup{state_block};

  if (FAILED(state_block->Capture()))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "IDirect3DStateBlock9::Capture failed."});
  }

  OnFrameGeneric(hadesmem::cerberus::RenderApi::kD3D9, device);

  if (FAILED(state_block->Apply()))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "IDirect3DStateBlock9::Apply failed."});
  }
}

void OnFrameOpenGL32(HDC device)
{
  HandleOnFrameOpenGL32(device);

  OnFrameGeneric(hadesmem::cerberus::RenderApi::kOpenGL32, device);
}

void OnResetD3D9(IDirect3DDevice9* device,
                 D3DPRESENT_PARAMETERS* presentation_parameters)
{
  HandleOnResetD3D9(device, presentation_parameters);
}
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

bool& GetGuiVisible() HADESMEM_DETAIL_NOEXCEPT
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

void InitializeRender()
{
  auto& dxgi = GetDXGIInterface();
  dxgi.RegisterOnFrame(OnFrameDXGI);

  auto& d3d9 = GetD3D9Interface();
  d3d9.RegisterOnFrame(OnFrameD3D9);
  d3d9.RegisterOnReset(OnResetD3D9);

  auto& opengl32 = GetOpenGL32Interface();
  opengl32.RegisterOnFrame(OnFrameOpenGL32);
}
}
}
