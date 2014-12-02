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
#include "d3d9.hpp"
#include "d3d10.hpp"
#include "d3d11.hpp"
#include "dxgi.hpp"
#include "hook_disabler.hpp"
#include "input.hpp"
#include "opengl.hpp"
#include "plugin.hpp"
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

int& GetShowCursorCount() HADESMEM_DETAIL_NOEXCEPT
{
  static int show_cursor_count{};
  return show_cursor_count;
}

std::pair<bool, HCURSOR>& GetOldCursor() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<bool, HCURSOR> old_cursor{};
  return old_cursor;
}

std::pair<bool, POINT>& GetOldCursorPos() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<bool, POINT> cursor_pos{};
  return cursor_pos;
}

RECT& GetOldClipCursor() HADESMEM_DETAIL_NOEXCEPT
{
  static RECT old_clip_cursor{};
  return old_clip_cursor;
}

bool& GetAntTweakBarVisible() HADESMEM_DETAIL_NOEXCEPT
{
  static bool visible = false;
  return visible;
}

struct WndProcInputMsg
{
  HWND hwnd_;
  UINT msg_;
  WPARAM wparam_;
  LPARAM lparam_;
};

std::queue<WndProcInputMsg>& GetWndProcInputMsgQueue()
{
  static std::queue<WndProcInputMsg> queue;
  return queue;
}

std::recursive_mutex& GetWndProcInputMsgQueueMutex()
{
  static std::recursive_mutex mutex;
  return mutex;
}

void SetAllTweakBarVisibility(bool visible)
{
  for (int i = 0; i < ::TwGetBarCount(); ++i)
  {
    auto const bar = ::TwGetBarByIndex(i);
    auto const name = ::TwGetBarName(bar);
    auto const define = std::string(name) + " visible=" +
                        (visible ? std::string("true") : std::string("false"));
    ::TwDefine(define.c_str());
  }
}

void SetOrRestoreCursor(bool visible)
{
  hadesmem::cerberus::HookDisabler disable_set_cursor_hook{
    &hadesmem::cerberus::GetDisableSetCursorHook()};

  auto& old_cursor = GetOldCursor();

  if (visible)
  {
    HCURSOR const arrow_cursor = ::LoadCursorW(nullptr, IDC_ARROW);
    if (!arrow_cursor)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"LoadCursorW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    HADESMEM_DETAIL_TRACE_A("Setting arrow cursor.");
    old_cursor.second = ::SetCursor(arrow_cursor);
  }
  else if (old_cursor.first)
  {
    HADESMEM_DETAIL_TRACE_A("Setting old cursor.");
    old_cursor.second = ::SetCursor(old_cursor.second);
  }

  old_cursor.first = true;
}

void SaveCurrentCursorPos()
{
  auto& old_cursor_pos = GetOldCursorPos();

  hadesmem::cerberus::HookDisabler disable_get_cursor_pos_hook{
    &hadesmem::cerberus::GetDisableGetCursorPosHook()};

  POINT cur_cursor_pos{};
  if (!::GetCursorPos(&cur_cursor_pos))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetCursorPos failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  old_cursor_pos.first = true;
  old_cursor_pos.second = cur_cursor_pos;
}

void ClearOldCursorPos()
{
  auto& old_cursor_pos = GetOldCursorPos();
  old_cursor_pos.first = false;
  old_cursor_pos.second = POINT{};
}

void RestoreOldCursorPos()
{
  auto& old_cursor_pos = GetOldCursorPos();

  if (!old_cursor_pos.first)
  {
    return;
  }

  hadesmem::cerberus::HookDisabler disable_set_cursor_pos_hook{
    &hadesmem::cerberus::GetDisableSetCursorPosHook()};

  if (!::SetCursorPos(old_cursor_pos.second.x, old_cursor_pos.second.y))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"SetCursorPos failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  ClearOldCursorPos();
}

void ShowCursor()
{
  hadesmem::cerberus::HookDisabler disable_show_cursor_hook{
    &hadesmem::cerberus::GetDisableShowCursorHook()};

  auto& show_cursor_count = GetShowCursorCount();
  do
  {
    HADESMEM_DETAIL_TRACE_A("Showing cursor.");
    ++show_cursor_count;
  } while (::ShowCursor(TRUE) < 0);
}

void HideCursor()
{
  hadesmem::cerberus::HookDisabler disable_show_cursor_hook{
    &hadesmem::cerberus::GetDisableShowCursorHook()};

  auto& show_cursor_count = GetShowCursorCount();
  while (show_cursor_count > 0)
  {
    HADESMEM_DETAIL_TRACE_A("Hiding cursor.");
    --show_cursor_count;
    ::ShowCursor(FALSE);
  }
}

void SaveCurrentClipCursor()
{
  hadesmem::cerberus::HookDisabler disable_get_clip_cursor_hook{
    &hadesmem::cerberus::GetDisableGetClipCursorHook()};

  RECT clip_cursor{};
  if (!::GetClipCursor(&clip_cursor))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetClipCursor failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  HADESMEM_DETAIL_TRACE_FORMAT_A("Saving current clip cursor: Left [%ld] Top "
                                 "[%ld] Right [%ld] Bottom [%ld]",
                                 clip_cursor.left,
                                 clip_cursor.top,
                                 clip_cursor.right,
                                 clip_cursor.bottom);

  auto& old_clip_cursor = GetOldClipCursor();
  old_clip_cursor = clip_cursor;
}

void SetNewClipCursor()
{
  RECT new_clip_cursor{};
  if (!::GetWindowRect(hadesmem::cerberus::GetCurrentWindow(),
                       &new_clip_cursor))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetWindowRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  hadesmem::cerberus::HookDisabler disable_clip_cursor_hook{
    &hadesmem::cerberus::GetDisableClipCursorHook()};

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Setting new clip cursor: Left [%ld] Top [%ld] Right [%ld] Bottom [%ld]",
    new_clip_cursor.left,
    new_clip_cursor.top,
    new_clip_cursor.right,
    new_clip_cursor.bottom);

  if (!::ClipCursor(&new_clip_cursor))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"ClipCursor failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
}

void RestoreOldClipCursor()
{
  hadesmem::cerberus::HookDisabler disable_clip_cursor_hook{
    &hadesmem::cerberus::GetDisableClipCursorHook()};

  auto& clip_cursor = GetOldClipCursor();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Restoring old clip cursor: Left [%ld] Top "
                                 "[%ld] Right [%ld] Bottom [%ld]",
                                 clip_cursor.left,
                                 clip_cursor.top,
                                 clip_cursor.right,
                                 clip_cursor.bottom);

  if (!::ClipCursor(&clip_cursor))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"ClipCursor failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
}

void SetAntTweakBarVisible(bool visible, bool old_visible)
{
  HADESMEM_DETAIL_TRACE_A("Setting tweak bars visibility attribute.");

  SetAllTweakBarVisibility(visible);

  if (visible != old_visible)
  {
    SetOrRestoreCursor(visible);

    if (visible)
    {
      SaveCurrentCursorPos();

      ShowCursor();

      SaveCurrentClipCursor();

      SetNewClipCursor();
    }
    else
    {
      RestoreOldCursorPos();

      HideCursor();

      RestoreOldClipCursor();
    }
  }
  else
  {
    ClearOldCursorPos();

    SetNewClipCursor();
  }

  HADESMEM_DETAIL_TRACE_A("Finished.");
}

void ToggleAntTweakBarVisible()
{
  auto& visible = GetAntTweakBarVisible();
  visible = !visible;
  HADESMEM_DETAIL_TRACE_A(visible ? "Showing all tweak bars."
                                  : "Hiding all tweak bars.");
  SetAntTweakBarVisible(visible, !visible);
}

void HandleInputQueueEntry(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  hadesmem::cerberus::HookDisabler disable_set_cursor_hook{
    &hadesmem::cerberus::GetDisableSetCursorHook()};

  ::TwEventWin(hwnd, msg, wparam, lparam);
}

void HandleInputQueue()
{
  auto& queue = GetWndProcInputMsgQueue();
  auto& mutex = GetWndProcInputMsgQueueMutex();
  std::lock_guard<std::recursive_mutex> lock{mutex};
  while (!queue.empty())
  {
    WndProcInputMsg msg = queue.front();
    HandleInputQueueEntry(msg.hwnd_, msg.msg_, msg.wparam_, msg.lparam_);
    queue.pop();
  }
}

void WindowProcCallback(
  HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool* handled)
{
  {
    auto& queue = GetWndProcInputMsgQueue();
    auto& mutex = GetWndProcInputMsgQueueMutex();
    std::lock_guard<std::recursive_mutex> lock{mutex};
    queue.push(WndProcInputMsg{hwnd, msg, wparam, lparam});
  }

  if (msg == WM_KEYDOWN && !((lparam >> 30) & 1) && wparam == VK_F9 &&
      ::GetAsyncKeyState(VK_SHIFT) & 0x8000)
  {
    ToggleAntTweakBarVisible();
    *handled = true;
    return;
  }

  auto& visible = GetAntTweakBarVisible();
  bool const blocked_msg = msg == WM_INPUT ||
                           (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) ||
                           (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST);
  // Window #0 will always exist if TwInit has completed successfully.
  if (visible && blocked_msg && ::TwWindowExists(0))
  {
    *handled = true;
    return;
  }
}

void OnSetCursor(HCURSOR cursor,
                 bool* handled,
                 HCURSOR* retval) HADESMEM_DETAIL_NOEXCEPT
{
  auto& old_cursor = GetOldCursor();
  HCURSOR const old_cursor_raw = old_cursor.second;
  old_cursor.first = true;
  old_cursor.second = cursor;

  if (GetAntTweakBarVisible())
  {
    *retval = old_cursor_raw;
    *handled = true;
  }
}

void OnDirectInput(bool* handled) HADESMEM_DETAIL_NOEXCEPT
{
  if (GetAntTweakBarVisible())
  {
    *handled = true;
  }
}

void OnGetCursorPos(LPPOINT point, bool* handled) HADESMEM_DETAIL_NOEXCEPT
{
  if (GetAntTweakBarVisible() && point)
  {
    auto& old_cursor_pos = GetOldCursorPos();
    point->x = old_cursor_pos.second.x;
    point->y = old_cursor_pos.second.y;

    *handled = true;
  }
}

void OnSetCursorPos(int x, int y, bool* handled) HADESMEM_DETAIL_NOEXCEPT
{
  if (GetAntTweakBarVisible())
  {
    auto& old_cursor_pos = GetOldCursorPos();
    old_cursor_pos.first = true;
    old_cursor_pos.second.x = x;
    old_cursor_pos.second.y = y;

    *handled = true;
  }
}

void
  OnShowCursor(BOOL show, bool* handled, int* retval) HADESMEM_DETAIL_NOEXCEPT
{
  if (GetAntTweakBarVisible())
  {
    auto& show_cursor_count = GetShowCursorCount();
    if (show)
    {
      ++show_cursor_count;
    }
    else
    {
      --show_cursor_count;
    }

    *retval = show_cursor_count;
    *handled = true;
  }
}

void OnClipCursor(RECT const* rect,
                  bool* handled,
                  BOOL* retval) HADESMEM_DETAIL_NOEXCEPT
{
  if (GetAntTweakBarVisible() && rect)
  {
    auto& clip_cursor = GetOldClipCursor();
    clip_cursor = *rect;
    *retval = TRUE;
    *handled = true;
  }
}

void OnGetClipCursor(RECT* rect,
                     bool* handled,
                     BOOL* retval) HADESMEM_DETAIL_NOEXCEPT
{
  if (GetAntTweakBarVisible() && rect)
  {
    auto& clip_cursor = GetOldClipCursor();
    *rect = clip_cursor;
    *retval = TRUE;
    *handled = true;
  }
}

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

  auto& visible = GetAntTweakBarVisible();

  HADESMEM_DETAIL_TRACE_A("Calling AntTweakBar initialization callbacks.");

  auto& ant_tweak_bar = hadesmem::cerberus::GetAntTweakBarInterface();
  ant_tweak_bar.CallOnInitialize();

  HADESMEM_DETAIL_TRACE_A("Setting tweak bar visibilty.");

  SetAntTweakBarVisible(visible, visible);

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
    __uuidof(render_info.device_),
    reinterpret_cast<void**>(&render_info.device_));
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
    __uuidof(render_info.device_),
    reinterpret_cast<void**>(&render_info.device_));
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

  HandleInputQueue();

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

  auto& input = GetInputInterface();
  input.RegisterOnWndProcMsg(WindowProcCallback);
  input.RegisterOnSetCursor(OnSetCursor);
  input.RegisterOnGetCursorPos(OnGetCursorPos);
  input.RegisterOnSetCursorPos(OnSetCursorPos);
  input.RegisterOnShowCursor(OnShowCursor);
  input.RegisterOnClipCursor(OnClipCursor);
  input.RegisterOnGetClipCursor(OnGetClipCursor);
  input.RegisterOnDirectInput(OnDirectInput);

  RegisterOnUnloadPlugins(OnUnloadPlugins);
}
}
}
