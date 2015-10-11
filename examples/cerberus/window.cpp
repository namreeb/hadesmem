// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "window.hpp"

#include <cstdint>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/patcher.hpp>

#include "callbacks.hpp"
#include "helpers.hpp"
#include "hook_counter.hpp"
#include "main.hpp"

// TODO: Hook DispatchMessage, PeekMessage, GetMessage, etc. instead of using
// window procedure hooks.

// TODO: Support multiple windows.

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnWndProcMsgCallback>&
  GetOnWndProcMsgCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnWndProcMsgCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<
  hadesmem::cerberus::OnGetForegroundWindowCallback>&
  GetOnGetForegroundWindowCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnGetForegroundWindowCallback> callbacks;
  return callbacks;
}

struct WindowInfo
{
  HWND old_hwnd_{nullptr};
  WNDPROC old_wndproc_{nullptr};
  bool hooked_{false};
};

WindowInfo& GetWindowInfo() noexcept
{
  static WindowInfo window_info;
  return window_info;
}

bool& GetEnableForegroundWindowSpoof()
{
  static bool enabled_ = false;
  return enabled_;
}

class WindowImpl : public hadesmem::cerberus::WindowInterface
{
public:
  WindowImpl()
  {
    GetWindowInfo();
  }

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

  virtual std::size_t RegisterOnGetForegroundWindow(
    std::function<hadesmem::cerberus::OnGetForegroundWindowCallback> const&
      callback) final
  {
    auto& callbacks = GetOnGetForegroundWindowCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnGetForegroundWindow(std::size_t id) final
  {
    auto& callbacks = GetOnGetForegroundWindowCallbacks();
    return callbacks.Unregister(id);
  }

  virtual HWND GetCurrentWindow() const final
  {
    return GetWindowInfo().old_hwnd_;
  }
};

std::unique_ptr<hadesmem::PatchDetour<decltype(&::GetForegroundWindow)>>&
  GetGetForegroundWindowDetour() noexcept
{
  static std::unique_ptr<
    hadesmem::PatchDetour<decltype(&::GetForegroundWindow)>> detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetUser32Module() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

extern "C" HWND WINAPI
  GetForegroundWindowDetour(hadesmem::PatchDetourBase* detour) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const& callbacks = GetOnGetForegroundWindowCallbacks();
  bool handled = false;
  HWND retval = nullptr;
  callbacks.Run(&handled, &retval);

  if (handled)
  {
    HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
      "Spoofing foreground window with [%p].", retval);
    return retval;
  }

  auto const get_foreground_window =
    detour->GetTrampolineT<decltype(&::GetForegroundWindow)>();
  last_error_preserver.Revert();
  auto const ret = get_foreground_window();
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
}

LRESULT CALLBACK WindowProc(HWND hwnd,
                            UINT msg,
                            WPARAM wparam,
                            LPARAM lparam) noexcept
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
WindowInterface& GetWindowInterface() noexcept
{
  static WindowImpl window_impl;
  return window_impl;
}

void InitializeWindow()
{
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(
    L"USER32", DetourUser32ForWindow, UndetourUser32ForWindow, GetUser32Module);
}

void DetourUser32ForWindow(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetUser32Module();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"user32", base, module))
  {
    DetourFunc(process,
               base,
               "GetForegroundWindow",
               GetGetForegroundWindowDetour(),
               GetForegroundWindowDetour);
  }
}

void UndetourUser32ForWindow(bool remove)
{
  auto& module = GetUser32Module();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"user32", module))
  {
    UndetourFunc(
      L"GetForegroundWindow", GetGetForegroundWindowDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
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

bool IsWindowHooked() noexcept
{
  return GetWindowInfo().hooked_;
}
}
}
