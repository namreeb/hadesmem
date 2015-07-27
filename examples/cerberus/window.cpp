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

std::unique_ptr<hadesmem::PatchDetour<decltype(&::DispatchMessageA)>>&
  GetDispatchMessageADetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::DispatchMessageA)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::DispatchMessageW)>>&
  GetDispatchMessageWDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::DispatchMessageW)>>
    detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetUser32Module() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

extern "C" HWND WINAPI GetForegroundWindowDetour(
  hadesmem::PatchDetourBase* detour) noexcept
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

extern "C" LRESULT WINAPI
  DispatchMessageADetour(hadesmem::PatchDetourBase* detour,
                         MSG const* msg) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", msg);

  auto const& callbacks = GetOnWndProcMsgCallbacks();
  bool handled = false;
  callbacks.Run(msg->hwnd, msg->message, msg->wParam, msg->lParam, &handled);

  if (handled)
  {
    return 0;
  }

  auto const dispatch_message_a =
    detour->GetTrampolineT<decltype(&::DispatchMessageA)>();
  last_error_preserver.Revert();
  auto const ret = dispatch_message_a(msg);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
}

extern "C" LRESULT WINAPI
  DispatchMessageWDetour(hadesmem::PatchDetourBase* detour,
                         MSG const* msg) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", msg);

  auto const& callbacks = GetOnWndProcMsgCallbacks();
  bool handled = false;
  callbacks.Run(msg->hwnd, msg->message, msg->wParam, msg->lParam, &handled);

  if (handled)
  {
    return 0;
  }

  auto const dispatch_message_w =
    detour->GetTrampolineT<decltype(&::DispatchMessageW)>();
  last_error_preserver.Revert();
  auto const ret = dispatch_message_w(msg);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
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
    DetourFunc(process,
               base,
               "DispatchMessageA",
               GetDispatchMessageADetour(),
               DispatchMessageADetour);
    DetourFunc(process,
               base,
               "DispatchMessageW",
               GetDispatchMessageWDetour(),
               DispatchMessageWDetour);
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
    UndetourFunc(L"DispatchMessageA", GetDispatchMessageADetour(), remove);
    UndetourFunc(L"DispatchMessageW", GetDispatchMessageWDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}

// TODO: Fix all this now that we're using a DispatchMessage hook instead of
// subclassing windows.
void HandleWindowChange(HWND wnd)
{
  WindowInfo& window_info = GetWindowInfo();

  if (wnd == nullptr)
  {
    HADESMEM_DETAIL_TRACE_A("Clearing window hook data.");
    window_info.old_hwnd_ = nullptr;
    window_info.hooked_ = false;

    return;
  }

  if (!window_info.hooked_)
  {
    window_info.old_hwnd_ = wnd;
    window_info.hooked_ = true;
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
