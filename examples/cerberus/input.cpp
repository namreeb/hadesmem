// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "input.hpp"

#include <cstdint>

#include <windows.h>
#include <hadesmem/detail/warning_disable_prefix.hpp>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/detour_ref_counter.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
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

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnSetCursorCallback>&
  GetOnSetCursorCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnSetCursorCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnGetCursorPosCallback>&
  GetOnGetCursorPosCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnGetCursorPosCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnSetCursorPosCallback>&
  GetOnSetCursorPosCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnSetCursorPosCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnShowCursorCallback>&
  GetOnShowCursorCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnShowCursorCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnDirectInputCallback>&
  GetOnDirectInputCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnDirectInputCallback> callbacks;
  return callbacks;
}

class InputImpl : public hadesmem::cerberus::InputInterface
{
public:
  ~InputImpl()
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

  virtual std::size_t RegisterOnSetCursor(
    std::function<hadesmem::cerberus::OnSetCursorCallback> const& callback)
    final
  {
    auto& callbacks = GetOnSetCursorCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnSetCursor(std::size_t id) final
  {
    auto& callbacks = GetOnSetCursorCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnGetCursorPos(
    std::function<hadesmem::cerberus::OnGetCursorPosCallback> const& callback)
    final
  {
    auto& callbacks = GetOnGetCursorPosCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnGetCursorPos(std::size_t id) final
  {
    auto& callbacks = GetOnGetCursorPosCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnSetCursorPos(
    std::function<hadesmem::cerberus::OnSetCursorPosCallback> const& callback)
    final
  {
    auto& callbacks = GetOnSetCursorPosCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnSetCursorPos(std::size_t id) final
  {
    auto& callbacks = GetOnSetCursorPosCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnShowCursor(
    std::function<hadesmem::cerberus::OnShowCursorCallback> const& callback)
    final
  {
    auto& callbacks = GetOnShowCursorCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnShowCursor(std::size_t id) final
  {
    auto& callbacks = GetOnShowCursorCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnDirectInput(
    std::function<hadesmem::cerberus::OnDirectInputCallback> const& callback)
    final
  {
    auto& callbacks = GetOnDirectInputCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnDirectInput(std::size_t id) final
  {
    auto& callbacks = GetOnDirectInputCallbacks();
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

std::unique_ptr<hadesmem::PatchDetour>&
  GetDirectInput8CreateDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDirectInput8ACreateDeviceDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDirectInput8WCreateDeviceDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDirectInputDevice8AGetDeviceStateDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDirectInputDevice8WGetDeviceStateDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDirectInputDevice8AGetDeviceDataDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDirectInputDevice8WGetDeviceDataDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetDirectInput8Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetSetCursorDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetGetCursorPosDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetSetCursorPosDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetShowCursorDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetUser32Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
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

HRESULT WINAPI
  IDirectInputDevice8AGetDeviceStateDetour(IDirectInputDeviceA* device,
                                           DWORD data_len,
                                           LPVOID data) HADESMEM_DETAIL_NOEXCEPT
{
  auto const& callbacks = GetOnDirectInputCallbacks();
  bool handled = false;
  callbacks.Run(&handled);

  if (handled)
  {
    return E_FAIL;
  }

  auto& detour = GetIDirectInputDevice8AGetDeviceDataDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const get_device_data =
    detour
      ->GetTrampoline<decltype(&IDirectInputDevice8AGetDeviceStateDetour)>();
  last_error_preserver.Revert();
  auto const ret = get_device_data(device, data_len, data);
  last_error_preserver.Update();

  return ret;
}

HRESULT WINAPI
  IDirectInputDevice8WGetDeviceStateDetour(IDirectInputDeviceW* device,
                                           DWORD data_len,
                                           LPVOID data) HADESMEM_DETAIL_NOEXCEPT
{
  auto const& callbacks = GetOnDirectInputCallbacks();
  bool handled = false;
  callbacks.Run(&handled);

  if (handled)
  {
    return E_FAIL;
  }

  auto& detour = GetIDirectInputDevice8WGetDeviceStateDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const get_device_data =
    detour
      ->GetTrampoline<decltype(&IDirectInputDevice8WGetDeviceStateDetour)>();
  last_error_preserver.Revert();
  auto const ret = get_device_data(device, data_len, data);
  last_error_preserver.Update();

  return ret;
}

HRESULT WINAPI
  IDirectInputDevice8AGetDeviceDataDetour(IDirectInputDeviceA* device,
                                          DWORD cb_object_data,
                                          LPDIDEVICEOBJECTDATA dev_obj_data,
                                          LPDWORD in_out,
                                          DWORD flags) HADESMEM_DETAIL_NOEXCEPT
{
  auto const& callbacks = GetOnDirectInputCallbacks();
  bool handled = false;
  callbacks.Run(&handled);

  if (handled)
  {
    return E_FAIL;
  }

  auto& detour = GetIDirectInputDevice8AGetDeviceDataDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const get_device_data =
    detour->GetTrampoline<decltype(&IDirectInputDevice8AGetDeviceDataDetour)>();
  last_error_preserver.Revert();
  auto const ret =
    get_device_data(device, cb_object_data, dev_obj_data, in_out, flags);
  last_error_preserver.Update();

  return ret;
}

HRESULT WINAPI
  IDirectInputDevice8WGetDeviceDataDetour(IDirectInputDeviceW* device,
                                          DWORD cb_object_data,
                                          LPDIDEVICEOBJECTDATA dev_obj_data,
                                          LPDWORD in_out,
                                          DWORD flags) HADESMEM_DETAIL_NOEXCEPT
{
  auto const& callbacks = GetOnDirectInputCallbacks();
  bool handled = false;
  callbacks.Run(&handled);

  if (handled)
  {
    return E_FAIL;
  }

  auto& detour = GetIDirectInputDevice8WGetDeviceDataDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const get_device_data =
    detour->GetTrampoline<decltype(&IDirectInputDevice8WGetDeviceDataDetour)>();
  last_error_preserver.Revert();
  auto const ret =
    get_device_data(device, cb_object_data, dev_obj_data, in_out, flags);
  last_error_preserver.Update();

  return ret;
}

void DetourDirectInputDevice8(REFIID riid, void* device)
{
  HADESMEM_DETAIL_TRACE_A("Detouring IDirectInputDevice8.");

  try
  {
    auto const& process = hadesmem::cerberus::GetThisProcess();

    if (riid == IID_IDirectInputDevice8A)
    {
      hadesmem::cerberus::DetourFunc(
        process,
        L"IDirectInputDevice8A::GetDeviceState",
        device,
        9,
        GetIDirectInputDevice8AGetDeviceStateDetour(),
        IDirectInputDevice8AGetDeviceStateDetour);

      hadesmem::cerberus::DetourFunc(
        process,
        L"IDirectInputDevice8A::GetDeviceData",
        device,
        10,
        GetIDirectInputDevice8AGetDeviceDataDetour(),
        IDirectInputDevice8AGetDeviceDataDetour);
    }
    else if (riid == IID_IDirectInputDevice8W)
    {
      hadesmem::cerberus::DetourFunc(
        process,
        L"IDirectInputDevice8W::GetDeviceState",
        device,
        9,
        GetIDirectInputDevice8WGetDeviceStateDetour(),
        IDirectInputDevice8WGetDeviceStateDetour);

      hadesmem::cerberus::DetourFunc(
        process,
        L"IDirectInputDevice8W::GetDeviceData",
        device,
        10,
        GetIDirectInputDevice8WGetDeviceDataDetour(),
        IDirectInputDevice8WGetDeviceDataDetour);
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("WARNING! Unknown IID.");
      HADESMEM_DETAIL_ASSERT(false);
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

HRESULT WINAPI
  IDirectInput8ACreateDeviceDetour(IDirectInput8A* input,
                                   REFGUID rguid,
                                   IDirectInputDeviceA** direct_input_device,
                                   LPUNKNOWN unk_outer) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDirectInput8ACreateDeviceDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p] [%p] [%p].",
                                 input,
                                 &rguid,
                                 direct_input_device,
                                 unk_outer);

  auto const create_device =
    detour->GetTrampoline<decltype(&IDirectInput8ACreateDeviceDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_device(input, rguid, direct_input_device, unk_outer);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (FAILED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
    return ret;
  }

  HADESMEM_DETAIL_TRACE_A("Succeeded.");

  DetourDirectInputDevice8(IID_IDirectInputDevice8A, *direct_input_device);

  return ret;
}

HRESULT WINAPI
  IDirectInput8WCreateDeviceDetour(IDirectInput8W* input,
                                   REFGUID rguid,
                                   IDirectInputDeviceW** direct_input_device,
                                   LPUNKNOWN unk_outer) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDirectInput8WCreateDeviceDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p] [%p] [%p].",
                                 input,
                                 &rguid,
                                 direct_input_device,
                                 unk_outer);

  auto const create_device =
    detour->GetTrampoline<decltype(&IDirectInput8WCreateDeviceDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_device(input, rguid, direct_input_device, unk_outer);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (FAILED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
    return ret;
  }

  HADESMEM_DETAIL_TRACE_A("Succeeded.");

  DetourDirectInputDevice8(IID_IDirectInputDevice8W, *direct_input_device);

  return ret;
}

void DetourDirectInput8(REFIID riid, void** direct_input)
{
  HADESMEM_DETAIL_TRACE_A("Detouring IDirectInput8.");

  try
  {
    auto const& process = hadesmem::cerberus::GetThisProcess();

    if (riid == IID_IDirectInput8A)
    {
      hadesmem::cerberus::DetourFunc(process,
                                     L"IDirectInput8A::CreateDevice",
                                     *direct_input,
                                     3,
                                     GetIDirectInput8ACreateDeviceDetour(),
                                     IDirectInput8ACreateDeviceDetour);
    }
    else if (riid == IID_IDirectInput8W)
    {
      hadesmem::cerberus::DetourFunc(process,
                                     L"IDirectInput8W::CreateDevice",
                                     *direct_input,
                                     3,
                                     GetIDirectInput8WCreateDeviceDetour(),
                                     IDirectInput8WCreateDeviceDetour);
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("WARNING! Unknown IID.");
      HADESMEM_DETAIL_ASSERT(false);
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

HRESULT WINAPI
  DirectInput8CreateDetour(HINSTANCE hinst,
                           DWORD version,
                           REFIID riid,
                           LPVOID* out,
                           LPUNKNOWN unk_outer) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetDirectInput8CreateDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%u] [%p] [%p] [%p].", hinst, version, &riid, out, unk_outer);
  auto const direct_input_8_create =
    detour->GetTrampoline<decltype(&DirectInput8CreateDetour)>();
  last_error_preserver.Revert();
  auto const ret = direct_input_8_create(hinst, version, riid, out, unk_outer);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (FAILED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
    return ret;
  }

  HADESMEM_DETAIL_TRACE_A("Succeeded.");

  DetourDirectInput8(riid, out);

  return ret;
}

HCURSOR WINAPI SetCursorDetour(HCURSOR cursor) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetSetCursorDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p].", cursor);

  if (!hadesmem::cerberus::GetDisableSetCursorHook())
  {
    auto const& callbacks = GetOnSetCursorCallbacks();
    bool handled = false;
    HCURSOR retval{};
    callbacks.Run(cursor, &handled, &retval);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_A("SetCursor handled. Not calling trampoline.");
      HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%p].", retval);
      return retval;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("SetCursor hook disabled, skipping callbacks.");
  }

  auto const set_cursor = detour->GetTrampoline<decltype(&SetCursorDetour)>();
  last_error_preserver.Revert();
  auto const ret = set_cursor(cursor);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%p].", ret);

  return ret;
}

BOOL WINAPI GetCursorPosDetour(LPPOINT point) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetGetCursorPosDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p].", point);

  if (!hadesmem::cerberus::GetDisableGetCursorPosHook())
  {
    auto const& callbacks = GetOnGetCursorPosCallbacks();
    bool handled = false;
    callbacks.Run(point, &handled);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_A("GetCursorPos handled. Not calling trampoline.");
      return TRUE;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("GetCursorPos hook disabled, skipping callbacks.");
  }

  auto const get_cursor_pos =
    detour->GetTrampoline<decltype(&GetCursorPosDetour)>();
  last_error_preserver.Revert();
  auto const ret = get_cursor_pos(point);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

BOOL WINAPI SetCursorPosDetour(int x, int y) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetSetCursorPosDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%d] [%d].", x, y);

  auto const& callbacks = GetOnSetCursorPosCallbacks();
  bool handled = false;
  callbacks.Run(x, y, &handled);

  if (handled)
  {
    HADESMEM_DETAIL_TRACE_A("SetCursorPos handled. Not calling trampoline.");
    return TRUE;
  }

  auto const set_cursor_pos =
    detour->GetTrampoline<decltype(&SetCursorPosDetour)>();
  last_error_preserver.Revert();
  auto const ret = set_cursor_pos(x, y);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

int WINAPI ShowCursorDetour(BOOL show) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetShowCursorDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%d].", show);

  if (!hadesmem::cerberus::GetDisableShowCursorHook())
  {
    auto const& callbacks = GetOnShowCursorCallbacks();
    bool handled = false;
    int retval{};
    callbacks.Run(show, &handled, &retval);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_A("ShowCursor handled. Not calling trampoline.");
      HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%d].", retval);
      return retval;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("ShowCursor hook disabled, skipping callbacks.");
  }

  auto const show_cursor = detour->GetTrampoline<decltype(&ShowCursorDetour)>();
  last_error_preserver.Revert();
  auto const ret = show_cursor(show);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%d].", ret);

  return ret;
}
}

namespace hadesmem
{
namespace cerberus
{
InputInterface& GetInputInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static InputImpl input_impl;
  return input_impl;
}

void InitializeInput()
{
  InitializeSupportForModule(L"dinput8",
                             DetourDirectInput8,
                             UndetourDirectInput8,
                             GetDirectInput8Module);

  InitializeSupportForModule(
    L"user32", DetourUser32, UndetourUser32, GetUser32Module);
}

void DetourDirectInput8(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetDirectInput8Module();
  if (CommonDetourModule(process, L"dinput8", base, module))
  {
    DetourFunc(process,
               base,
               "DirectInput8Create",
               GetDirectInput8CreateDetour(),
               DirectInput8CreateDetour);
  }
}

void UndetourDirectInput8(bool remove)
{
  auto& module = GetDirectInput8Module();
  if (CommonUndetourModule(L"dinput8", module))
  {
    UndetourFunc(L"DirectInput8Create", GetDirectInput8CreateDetour(), remove);
    UndetourFunc(L"IDirectInput8A::CreateDevice",
                 GetIDirectInput8ACreateDeviceDetour(),
                 remove);
    UndetourFunc(L"IDirectInput8W::CreateDevice",
                 GetIDirectInput8WCreateDeviceDetour(),
                 remove);
    UndetourFunc(L"IDirectInputDevice8A::GetDeviceState",
                 GetIDirectInputDevice8AGetDeviceStateDetour(),
                 remove);
    UndetourFunc(L"IDirectInputDevice8W::GetDeviceState",
                 GetIDirectInputDevice8WGetDeviceStateDetour(),
                 remove);
    UndetourFunc(L"IDirectInputDevice8A::GetDeviceData",
                 GetIDirectInputDevice8AGetDeviceDataDetour(),
                 remove);
    UndetourFunc(L"IDirectInputDevice8W::GetDeviceData",
                 GetIDirectInputDevice8WGetDeviceDataDetour(),
                 remove);

    module = std::make_pair(nullptr, 0);
  }
}

void DetourUser32(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetUser32Module();
  if (CommonDetourModule(process, L"user32", base, module))
  {
    DetourFunc(
      process, base, "SetCursor", GetSetCursorDetour(), SetCursorDetour);
    DetourFunc(process,
               base,
               "GetCursorPos",
               GetGetCursorPosDetour(),
               GetCursorPosDetour);
    DetourFunc(process,
               base,
               "SetCursorPos",
               GetSetCursorPosDetour(),
               SetCursorPosDetour);
    DetourFunc(
      process, base, "ShowCursor", GetShowCursorDetour(), ShowCursorDetour);
  }
}

void UndetourUser32(bool remove)
{
  auto& module = GetDirectInput8Module();
  if (CommonUndetourModule(L"user32", module))
  {
    UndetourFunc(L"SetCursor", GetSetCursorDetour(), remove);
    UndetourFunc(L"GetCursorPos", GetGetCursorPosDetour(), remove);
    UndetourFunc(L"SetCursorPos", GetSetCursorPosDetour(), remove);
    UndetourFunc(L"ShowCursor", GetShowCursorDetour(), remove);

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

bool IsWindowHooked() HADESMEM_DETAIL_NOEXCEPT
{
  return GetWindowInfo().hooked_;
}

HWND GetCurrentWindow() HADESMEM_DETAIL_NOEXCEPT
{
  return GetWindowInfo().old_hwnd_;
}

bool& GetDisableSetCursorHook() HADESMEM_DETAIL_NOEXCEPT
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool disable_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool disable_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  return disable_hook;
}

bool& GetDisableGetCursorPosHook() HADESMEM_DETAIL_NOEXCEPT
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool disable_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool disable_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  return disable_hook;
}

bool& GetDisableShowCursorHook() HADESMEM_DETAIL_NOEXCEPT
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool disable_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool disable_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  return disable_hook;
}
}
}
