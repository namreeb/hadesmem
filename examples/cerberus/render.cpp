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

#include "ant_tweak_bar.hpp"
#include "callbacks.hpp"
#include "cursor.hpp"
#include "d3d9.hpp"
#include "d3d10.hpp"
#include "d3d11.hpp"
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

  virtual hadesmem::cerberus::AntTweakBarInterface*
    GetAntTweakBarInterface() final
  {
    return &hadesmem::cerberus::GetAntTweakBarInterface();
  }
};

struct RenderInfoDXGI
{
  bool first_time_{true};
  bool tw_initialized_{false};
  bool wnd_hooked_{false};
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

struct RenderInfoD3D9
{
  bool first_time_{true};
  bool tw_initialized_{false};
  bool wnd_hooked_{false};
  IDirect3DDevice9* device_{};
};

RenderInfoD3D9& GetRenderInfoD3D9() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoD3D9 render_info;
  return render_info;
}

struct RenderInfoOpenGL32
{
  bool first_time_{true};
  bool tw_initialized_{false};
  bool wnd_hooked_{false};
  HDC device_{};
};

RenderInfoOpenGL32& GetRenderInfoOpenGL32() HADESMEM_DETAIL_NOEXCEPT
{
  static RenderInfoOpenGL32 render_info;
  return render_info;
}

void SetAntTweakBarUninitialized() HADESMEM_DETAIL_NOEXCEPT
{
  GetRenderInfoD3D9().tw_initialized_ = false;
  GetRenderInfoD3D10().tw_initialized_ = false;
  GetRenderInfoD3D11().tw_initialized_ = false;
  GetRenderInfoOpenGL32().tw_initialized_ = false;
}

bool InitializeWndprocHook(RenderInfoDXGI& render_info)
{
  if (!hadesmem::cerberus::IsWindowHooked())
  {
    DXGI_SWAP_CHAIN_DESC desc;
    auto const get_desc_hr = render_info.swap_chain_->GetDesc(&desc);
    if (FAILED(get_desc_hr))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"GetDesc failed."}
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
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Window is already hooked. Skipping hook request.");
  }

  return false;
}

bool InitializeWndprocHook(RenderInfoD3D9& render_info)
{
  if (!hadesmem::cerberus::IsWindowHooked())
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
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Window is already hooked. Skipping hook request.");
  }

  return false;
}

bool InitializeWndprocHook(RenderInfoOpenGL32& render_info)
{
  if (!hadesmem::cerberus::IsWindowHooked())
  {
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
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Window is already hooked. Skipping hook request.");
  }

  return false;
}

std::string& GetPluginPathTw()
{
  static std::string path;
  return path;
}

void TW_CALL CopyStdStringToClientTw(std::string& dst, const std::string& src)
{
  dst = src;
}

void TW_CALL LoadPluginCallbackTw(void* /*client_data*/)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Path: %s.", GetPluginPathTw().c_str());

  try
  {
    hadesmem::cerberus::LoadPlugin(
      hadesmem::detail::MultiByteToWideChar(GetPluginPathTw()));
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to load plugin.");
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
  }
}

void TW_CALL UnloadPluginCallbackTw(void* /*client_data*/)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Path: %s.", GetPluginPathTw().c_str());

  try
  {
    hadesmem::cerberus::UnloadPlugin(
      hadesmem::detail::MultiByteToWideChar(GetPluginPathTw()));
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to unload plugin.");
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
  }
}

void InitializeAntTweakBar(TwGraphAPI api, void* device, bool& initialized)
{
  if (hadesmem::cerberus::AntTweakBarInitializedAny())
  {
    HADESMEM_DETAIL_TRACE_A(
      "WARNING! AntTweakBar is already initialized. Skipping.");
    return;
  }

  HADESMEM_DETAIL_TRACE_A("Initializing AntTweakBar.");

  if (!::TwInit(api, device))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwInit failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  initialized = true;

  RECT wnd_rect{0, 0, 800, 600};
  if (auto const window = hadesmem::cerberus::GetCurrentWindow())
  {
    HADESMEM_DETAIL_TRACE_A("Have a window.");

    if (!::GetClientRect(window, &wnd_rect) || wnd_rect.right == 0 ||
        wnd_rect.bottom == 0)
    {
      HADESMEM_DETAIL_TRACE_A(
        "GetClientRect failed (or returned an invalid box).");

      wnd_rect = RECT{0, 0, 800, 600};
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Got client rect.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Do not have a window.");
  }
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Window size is %ldx%ld.", wnd_rect.right, wnd_rect.bottom);

  if (!::TwWindowSize(wnd_rect.right, wnd_rect.bottom))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwWindowSize failed."}
                        << hadesmem::ErrorCodeWinLast{last_error}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  ::TwCopyStdStringToClientFunc(CopyStdStringToClientTw);

  auto const bar = ::TwNewBar("HadesMem");
  if (!bar)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwNewBar failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  auto const load_button = ::TwAddButton(bar,
                                         "LoadPluginBtn",
                                         &LoadPluginCallbackTw,
                                         nullptr,
                                         " label='Load Plugin' ");
  if (!load_button)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwAddButton failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  auto const unload_button = ::TwAddButton(bar,
                                           "UnloadPluginBtn",
                                           &UnloadPluginCallbackTw,
                                           nullptr,
                                           " label='Unload Plugin' ");
  if (!unload_button)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwAddButton failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  auto const plugin_path = ::TwAddVarRW(bar,
                                        "LoadPluginPath",
                                        TW_TYPE_STDSTRING,
                                        &GetPluginPathTw(),
                                        " label='Plugin Path' ");
  if (!plugin_path)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwAddVarRW failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  HADESMEM_DETAIL_TRACE_A("Calling AntTweakBar initialization callbacks.");

  auto& ant_tweak_bar = hadesmem::cerberus::GetAntTweakBarInterface();
  ant_tweak_bar.CallOnInitialize();

  HADESMEM_DETAIL_TRACE_A("Setting tweak bar visibilty.");

  auto const visible = hadesmem::cerberus::GetGuiVisible();
  hadesmem::cerberus::SetGuiVisible(visible, visible);

  HADESMEM_DETAIL_TRACE_A("Finished.");
}

void CleanupAntTweakBar(bool& initialized)
{
  if (initialized)
  {
    HADESMEM_DETAIL_TRACE_A("Calling AntTweakBar cleanup callbacks.");

    auto& ant_tweak_bar = hadesmem::cerberus::GetAntTweakBarInterface();
    ant_tweak_bar.CallOnCleanup();

    HADESMEM_DETAIL_TRACE_A("Cleaning up AntTweakBar.");

    if (!::TwTerminate())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"TwTerminate failed."});
    }

    initialized = false;
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

  if (render_info.wnd_hooked_)
  {
    hadesmem::cerberus::HandleWindowChange(nullptr);
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

  CleanupAntTweakBar(render_info.tw_initialized_);

  if (render_info.wnd_hooked_)
  {
    hadesmem::cerberus::HandleWindowChange(nullptr);
  }

  render_info.wnd_hooked_ = false;
}

void HandleChangedDeviceD3D9(IDirect3DDevice9* device,
                             RenderInfoD3D9& render_info)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Got a new device. Old = %p, New = %p.", render_info.device_, device);
  render_info.device_ = device;

  render_info.first_time_ = true;

  CleanupAntTweakBar(render_info.tw_initialized_);

  if (render_info.wnd_hooked_)
  {
    hadesmem::cerberus::HandleWindowChange(nullptr);
  }

  render_info.wnd_hooked_ = false;
}

void HandleChangedDeviceOpenGL32(HDC device, RenderInfoOpenGL32& render_info)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Got a new device. Old = %p, New = %p.", render_info.device_, device);
  render_info.device_ = device;

  render_info.first_time_ = true;

  CleanupAntTweakBar(render_info.tw_initialized_);

  if (render_info.wnd_hooked_)
  {
    hadesmem::cerberus::HandleWindowChange(nullptr);
  }

  render_info.wnd_hooked_ = false;
}

void InitializeD3D11RenderInfo(RenderInfoD3D11& render_info)
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  render_info.first_time_ = false;

  auto const get_device_hr = render_info.swap_chain_->GetDevice(
    __uuidof(ID3D11Device), reinterpret_cast<void**>(&render_info.device_));
  if (FAILED(get_device_hr))
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "WARNING! IDXGISwapChain::GetDevice failed. HR = %08X.", get_device_hr);
    return;
  }

  render_info.wnd_hooked_ = InitializeWndprocHook(render_info);

  InitializeAntTweakBar(
    TW_DIRECT3D11, render_info.device_, render_info.tw_initialized_);

  HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
}

void InitializeD3D10RenderInfo(RenderInfoD3D10& render_info)
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  render_info.first_time_ = false;

  auto const get_device_hr = render_info.swap_chain_->GetDevice(
    __uuidof(ID3D10Device), reinterpret_cast<void**>(&render_info.device_));
  if (FAILED(get_device_hr))
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "WARNING! IDXGISwapChain::GetDevice failed. HR = %08X.", get_device_hr);
    return;
  }

  render_info.wnd_hooked_ = InitializeWndprocHook(render_info);

  InitializeAntTweakBar(
    TW_DIRECT3D10, render_info.device_, render_info.tw_initialized_);

  HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
}

void InitializeD3D9RenderInfo(RenderInfoD3D9& render_info)
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  render_info.first_time_ = false;

  render_info.wnd_hooked_ = InitializeWndprocHook(render_info);

  InitializeAntTweakBar(
    TW_DIRECT3D9, render_info.device_, render_info.tw_initialized_);

  HADESMEM_DETAIL_TRACE_A("Initialized successfully.");
}

void InitializeOpenGL32RenderInfo(RenderInfoOpenGL32& render_info)
{
  HADESMEM_DETAIL_TRACE_A("Initializing.");

  render_info.first_time_ = false;

  render_info.wnd_hooked_ = InitializeWndprocHook(render_info);

  InitializeAntTweakBar(
    TW_OPENGL, render_info.device_, render_info.tw_initialized_);

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

void HandleOnFrameOpenGL32(HDC device)
{
  auto& render_info = GetRenderInfoOpenGL32();
  if (render_info.device_ != device)
  {
    HandleChangedDeviceOpenGL32(device, render_info);
  }

  if (render_info.first_time_)
  {
    InitializeOpenGL32RenderInfo(render_info);
  }
}

void HandleOnResetD3D9(IDirect3DDevice9* device,
                       D3DPRESENT_PARAMETERS* /*presentation_parameters*/)
{
  auto& render_info = GetRenderInfoD3D9();

  if (device == render_info.device_)
  {
    HADESMEM_DETAIL_TRACE_A("Handling D3D9 device reset.");

    CleanupAntTweakBar(render_info.tw_initialized_);

    InitializeAntTweakBar(
      TW_DIRECT3D9, render_info.device_, render_info.tw_initialized_);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "WARNING! Detected reset on unknown device. Ours = %p, Theirs = %p.",
      render_info.device_,
      device);
  }
}

void OnFrameGeneric()
{
  auto& callbacks = GetOnFrameCallbacks();
  callbacks.Run();

  hadesmem::cerberus::HandleInputQueue();

  if (!::TwDraw())
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"TwDraw failed."});
  }
}

void OnFrameDXGI(IDXGISwapChain* swap_chain)
{
  HandleOnFrameD3D11(swap_chain);

  HandleOnFrameD3D10(swap_chain);

  OnFrameGeneric();
}

void OnFrameD3D9(IDirect3DDevice9* device)
{
  HandleOnFrameD3D9(device);

  OnFrameGeneric();
}

void OnFrameOpenGL32(HDC device)
{
  HandleOnFrameOpenGL32(device);

  OnFrameGeneric();
}

void OnResetD3D9(IDirect3DDevice9* device,
                 D3DPRESENT_PARAMETERS* presentation_parameters)
{
  HandleOnResetD3D9(device, presentation_parameters);
}

void OnUnloadPlugins()
{
  SetAntTweakBarUninitialized();
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

bool AntTweakBarInitializedAny() HADESMEM_DETAIL_NOEXCEPT
{
  return GetRenderInfoD3D9().tw_initialized_ ||
         GetRenderInfoD3D10().tw_initialized_ ||
         GetRenderInfoD3D11().tw_initialized_ ||
         GetRenderInfoOpenGL32().tw_initialized_;
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

  RegisterOnUnloadPlugins(OnUnloadPlugins);
}
}
}
