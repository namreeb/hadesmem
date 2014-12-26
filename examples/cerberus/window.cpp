// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "window.hpp"

#include <cstdint>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/trace.hpp>

#include "callbacks.hpp"
#include "main.hpp"

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnWndProcMsgCallback>&
  GetOnWndProcMsgCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnWndProcMsgCallback>
    callbacks;
  return callbacks;
}

class WindowImpl : public hadesmem::cerberus::WindowInterface
{
public:
  ~WindowImpl()
  {
    try
    {
      hadesmem::cerberus::HandleWindowChange(nullptr);
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);
    }
  }

  virtual std::size_t RegisterOnWndProcMsg(
    std::function<hadesmem::cerberus::OnWndProcMsgCallback> const& callback)
    final
  {
    auto& callbacks = GetOnWndProcMsgCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnWndProcMsg(std::size_t id) final
  {
    auto& callbacks = GetOnWndProcMsgCallbacks();
    return callbacks.Unregister(id);
  }
};

struct WindowInfo
{
  HWND old_hwnd_{nullptr};
  WNDPROC old_wndproc_{nullptr};
  bool hooked_{false};
};

WindowInfo& GetWindowInfo() HADESMEM_DETAIL_NOEXCEPT
{
  static WindowInfo window_info;
  return window_info;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
  HADESMEM_DETAIL_NOEXCEPT
{
  auto const& callbacks = GetOnWndProcMsgCallbacks();
  bool handled = false;
  callbacks.Run(hwnd, msg, wparam, lparam, &handled);

  if (handled)
  {
    return 0;
  }

  WindowInfo& window_info = GetWindowInfo();
  return window_info.old_wndproc_
           ? ::CallWindowProcW(
               window_info.old_wndproc_, hwnd, msg, wparam, lparam)
           : ::DefWindowProcW(hwnd, msg, wparam, lparam);
}
}

namespace hadesmem
{
namespace cerberus
{
WindowInterface& GetWindowInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static WindowImpl input_impl;
  return input_impl;
}

void HandleWindowChange(HWND wnd)
{
  WindowInfo& window_info = GetWindowInfo();

  if (wnd == nullptr)
  {
    if (window_info.hooked_ && window_info.old_wndproc_ != nullptr)
    {
      ::SetWindowLongPtrW(window_info.old_hwnd_,
                          GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(window_info.old_wndproc_));
      HADESMEM_DETAIL_TRACE_FORMAT_A("Reset window procedure located at %p.",
                                     window_info.old_wndproc_);
    }

    HADESMEM_DETAIL_TRACE_A("Clearing window hook data.");

    window_info.old_hwnd_ = nullptr;
    window_info.old_wndproc_ = nullptr;
    window_info.hooked_ = false;

    return;
  }

  if (!window_info.hooked_)
  {
    window_info.old_hwnd_ = wnd;
    ::SetLastError(0);
    window_info.old_wndproc_ = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
      wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowProc)));
    if (!window_info.old_wndproc_)
    {
      DWORD const last_error = ::GetLastError();
      if (last_error)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error{} << ErrorString{"SetWindowLongPtrW failed."}
                  << ErrorCodeWinLast{last_error});
      }
    }
    window_info.hooked_ = true;
    HADESMEM_DETAIL_TRACE_FORMAT_A("Replaced window procedure located at %p.",
                                   window_info.old_wndproc_);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Window is already hooked. Skipping hook request.");
  }
}

bool IsWindowHooked() HADESMEM_DETAIL_NOEXCEPT
{
  return GetWindowInfo().hooked_;
}

HWND GetCurrentWindow() HADESMEM_DETAIL_NOEXCEPT
{
  return GetWindowInfo().old_hwnd_;
}
}
}
