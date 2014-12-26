// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "input.hpp"

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <queue>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/str_conv.hpp>

#include "callbacks.hpp"
#include "cursor.hpp"
#include "direct_input.hpp"
#include "hook_disabler.hpp"
#include "render.hpp"
#include "main.hpp"
#include "window.hpp"

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnInputQueueEntry>&
  GetOnInputQueueEntryCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnInputQueueEntry>
    callbacks;
  return callbacks;
}

class InputImpl : public hadesmem::cerberus::InputInterface
{
public:
  virtual std::size_t RegisterOnInputQueueEntry(
    std::function<hadesmem::cerberus::OnInputQueueEntry> const& callback) final
  {
    auto& callbacks = GetOnInputQueueEntryCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnInputQueueEntry(std::size_t id) final
  {
    auto& callbacks = GetOnInputQueueEntryCallbacks();
    return callbacks.Unregister(id);
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

void ClipCursorWrap(RECT clip_cursor)
{
  hadesmem::cerberus::HookDisabler disable_clip_cursor_hook{
    &hadesmem::cerberus::GetDisableClipCursorHook()};

  if (!::ClipCursor(&clip_cursor))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"ClipCursor failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
}

void SetNewClipCursor()
{
  auto const wnd = hadesmem::cerberus::GetCurrentWindow();
  if (!::IsWindow(wnd))
  {
    HADESMEM_DETAIL_TRACE_A("WARNING! Invalid window.");
    return;
  }

  RECT new_clip_cursor{};
  if (!::GetWindowRect(wnd, &new_clip_cursor))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetWindowRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  auto const& old_clip_cursor = GetOldClipCursor();
  new_clip_cursor.left = (std::min)(old_clip_cursor.left, new_clip_cursor.left);
  new_clip_cursor.top = (std::min)(old_clip_cursor.top, new_clip_cursor.top);
  new_clip_cursor.right =
    (std::max)(old_clip_cursor.right, new_clip_cursor.right);
  new_clip_cursor.bottom =
    (std::max)(old_clip_cursor.bottom, new_clip_cursor.bottom);

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Setting new clip cursor: Left [%ld] Top [%ld] Right [%ld] Bottom [%ld]",
    new_clip_cursor.left,
    new_clip_cursor.top,
    new_clip_cursor.right,
    new_clip_cursor.bottom);

  ClipCursorWrap(new_clip_cursor);
}

void RestoreOldClipCursor()
{
  auto& clip_cursor = GetOldClipCursor();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Restoring old clip cursor: Left [%ld] Top "
                                 "[%ld] Right [%ld] Bottom [%ld]",
                                 clip_cursor.left,
                                 clip_cursor.top,
                                 clip_cursor.right,
                                 clip_cursor.bottom);

  ClipCursorWrap(clip_cursor);
}

void ToggleGuiVisible()
{
  auto const visible = !hadesmem::cerberus::GetGuiVisible();
  HADESMEM_DETAIL_TRACE_A(visible ? "Showing GUI." : "Hiding GUI.");
  hadesmem::cerberus::SetGuiVisible(visible, !visible);
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
    ToggleGuiVisible();
    *handled = true;
    return;
  }

  auto const visible = hadesmem::cerberus::GetGuiVisible();
  bool const blocked_msg = msg == WM_INPUT ||
                           (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) ||
                           (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST);
  if (visible && blocked_msg)
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

  if (hadesmem::cerberus::GetGuiVisible())
  {
    *retval = old_cursor_raw;
    *handled = true;
  }
}

void OnDirectInput(bool* handled) HADESMEM_DETAIL_NOEXCEPT
{
  if (hadesmem::cerberus::GetGuiVisible())
  {
    *handled = true;
  }
}

void OnGetCursorPos(LPPOINT point, bool* handled) HADESMEM_DETAIL_NOEXCEPT
{
  if (hadesmem::cerberus::GetGuiVisible() && point)
  {
    auto& old_cursor_pos = GetOldCursorPos();
    point->x = old_cursor_pos.second.x;
    point->y = old_cursor_pos.second.y;

    *handled = true;
  }
}

void OnSetCursorPos(int x, int y, bool* handled) HADESMEM_DETAIL_NOEXCEPT
{
  if (hadesmem::cerberus::GetGuiVisible())
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
  if (hadesmem::cerberus::GetGuiVisible())
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
  if (hadesmem::cerberus::GetGuiVisible() && rect)
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
  if (hadesmem::cerberus::GetGuiVisible() && rect)
  {
    auto& clip_cursor = GetOldClipCursor();
    *rect = clip_cursor;
    *retval = TRUE;
    *handled = true;
  }
}
}

namespace hadesmem
{
namespace cerberus
{
void SetGuiVisibleForInput(bool visible, bool old_visible)
{
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

    if (visible)
    {
      SetNewClipCursor();
    }
  }
}

void HandleInputQueue()
{
  auto& queue = GetWndProcInputMsgQueue();
  auto& mutex = GetWndProcInputMsgQueueMutex();
  std::lock_guard<std::recursive_mutex> lock{mutex};
  while (!queue.empty())
  {
    WndProcInputMsg msg = queue.front();
    auto& callbacks = GetOnInputQueueEntryCallbacks();
    callbacks.Run(msg.hwnd_, msg.msg_, msg.wparam_, msg.lparam_);
    queue.pop();
  }
}

InputInterface& GetInputInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static InputImpl input_impl;
  return input_impl;
}

void InitializeInput()
{
  auto& window = GetWindowInterface();
  window.RegisterOnWndProcMsg(WindowProcCallback);

  auto& cursor = GetCursorInterface();
  cursor.RegisterOnSetCursor(OnSetCursor);
  cursor.RegisterOnGetCursorPos(OnGetCursorPos);
  cursor.RegisterOnSetCursorPos(OnSetCursorPos);
  cursor.RegisterOnShowCursor(OnShowCursor);
  cursor.RegisterOnClipCursor(OnClipCursor);
  cursor.RegisterOnGetClipCursor(OnGetClipCursor);

  auto& direct_input = GetDirectInputInterface();
  direct_input.RegisterOnDirectInput(OnDirectInput);
}
}
}
