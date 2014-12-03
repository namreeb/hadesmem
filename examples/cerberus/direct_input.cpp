// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "direct_input.hpp"

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
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnDirectInputCallback>&
  GetOnDirectInputCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnDirectInputCallback> callbacks;
  return callbacks;
}

class DirectInputImpl : public hadesmem::cerberus::DirectInputInterface
{
public:
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

extern "C" HRESULT WINAPI
  IDirectInputDevice8AGetDeviceStateDetour(IDirectInputDeviceA* device,
                                           DWORD data_len,
                                           LPVOID data) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDirectInputDevice8AGetDeviceStateDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const& callbacks = GetOnDirectInputCallbacks();
  bool handled = false;
  callbacks.Run(&handled);

  if (handled)
  {
    return E_FAIL;
  }

  auto const get_device_data =
    detour
      ->GetTrampoline<decltype(&IDirectInputDevice8AGetDeviceStateDetour)>();
  last_error_preserver.Revert();
  auto const ret = get_device_data(device, data_len, data);
  last_error_preserver.Update();

  return ret;
}

extern "C" HRESULT WINAPI
  IDirectInputDevice8WGetDeviceStateDetour(IDirectInputDeviceW* device,
                                           DWORD data_len,
                                           LPVOID data) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDirectInputDevice8WGetDeviceStateDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const& callbacks = GetOnDirectInputCallbacks();
  bool handled = false;
  callbacks.Run(&handled);

  if (handled)
  {
    return E_FAIL;
  }

  auto const get_device_data =
    detour
      ->GetTrampoline<decltype(&IDirectInputDevice8WGetDeviceStateDetour)>();
  last_error_preserver.Revert();
  auto const ret = get_device_data(device, data_len, data);
  last_error_preserver.Update();

  return ret;
}

extern "C" HRESULT WINAPI
  IDirectInputDevice8AGetDeviceDataDetour(IDirectInputDeviceA* device,
                                          DWORD cb_object_data,
                                          LPDIDEVICEOBJECTDATA dev_obj_data,
                                          LPDWORD in_out,
                                          DWORD flags) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDirectInputDevice8AGetDeviceDataDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const& callbacks = GetOnDirectInputCallbacks();
  bool handled = false;
  callbacks.Run(&handled);

  if (handled)
  {
    return E_FAIL;
  }

  auto const get_device_data =
    detour->GetTrampoline<decltype(&IDirectInputDevice8AGetDeviceDataDetour)>();
  last_error_preserver.Revert();
  auto const ret =
    get_device_data(device, cb_object_data, dev_obj_data, in_out, flags);
  last_error_preserver.Update();

  return ret;
}

extern "C" HRESULT WINAPI
  IDirectInputDevice8WGetDeviceDataDetour(IDirectInputDeviceW* device,
                                          DWORD cb_object_data,
                                          LPDIDEVICEOBJECTDATA dev_obj_data,
                                          LPDWORD in_out,
                                          DWORD flags) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDirectInputDevice8WGetDeviceDataDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const& callbacks = GetOnDirectInputCallbacks();
  bool handled = false;
  callbacks.Run(&handled);

  if (handled)
  {
    return E_FAIL;
  }

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

extern "C" HRESULT WINAPI
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

extern "C" HRESULT WINAPI
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

extern "C" HRESULT WINAPI
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
}

namespace hadesmem
{
namespace cerberus
{
DirectInputInterface& GetDirectInputInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static DirectInputImpl input_impl;
  return input_impl;
}

void InitializeDirectInput()
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
}
}
