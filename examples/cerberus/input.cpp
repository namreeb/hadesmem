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

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnWndProcMsgCallback>&
  GetOnWndProcMsgCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnWndProcMsgCallback>
    callbacks;
  return callbacks;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
  HADESMEM_DETAIL_NOEXCEPT
{
  auto const& callbacks = GetOnWndProcMsgCallbacks();
  bool handled = false;
  callbacks.Run(hwnd, msg, wparam, lparam, handled);

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
  IDirectInputDevice8AGetDeviceDataDetour(IDirectInputDeviceA* device,
                                          DWORD cb_object_data,
                                          LPDIDEVICEOBJECTDATA dev_obj_data,
                                          LPDWORD in_out,
                                          DWORD flags)
{
  auto& detour = GetIDirectInputDevice8AGetDeviceDataDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
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
                                          DWORD flags)
{
  auto& detour = GetIDirectInputDevice8WGetDeviceDataDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
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
                                   LPUNKNOWN unk_outer)
{
  auto& detour = GetIDirectInput8ACreateDeviceDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
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
                                   LPUNKNOWN unk_outer)
{
  auto& detour = GetIDirectInput8WCreateDeviceDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
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
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
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
    return hadesmem::cerberus::RegisterOnWndProcMsgCallback(callback);
  }

  virtual void UnregisterOnWndProcMsg(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnWndProcMsgCallback(id);
  }
};
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
    UndetourFunc(L"IDirectInputDevice8A::GetDeviceData",
                 GetIDirectInputDevice8AGetDeviceDataDetour(),
                 remove);
    UndetourFunc(L"IDirectInputDevice8W::GetDeviceData",
                 GetIDirectInputDevice8WGetDeviceDataDetour(),
                 remove);

    module = std::make_pair(nullptr, 0);
  }
}

std::size_t RegisterOnWndProcMsgCallback(
  std::function<OnWndProcMsgCallback> const& callback)
{
  auto& callbacks = GetOnWndProcMsgCallbacks();
  return callbacks.Register(callback);
}

void UnregisterOnWndProcMsgCallback(std::size_t id)
{
  auto& callbacks = GetOnWndProcMsgCallbacks();
  return callbacks.Unregister(id);
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
