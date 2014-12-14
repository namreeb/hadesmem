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
#include "direct_input_8_a_proxy.hpp"
#include "direct_input_8_w_proxy.hpp"
#include "helpers.hpp"
#include "main.hpp"

namespace
{
class DirectInputImpl : public hadesmem::cerberus::DirectInputInterface
{
public:
  virtual std::size_t RegisterOnDirectInput(
    std::function<hadesmem::cerberus::OnDirectInputCallback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnDirectInputCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnDirectInput(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnDirectInputCallbacks();
    return callbacks.Unregister(id);
  }
};

std::unique_ptr<hadesmem::PatchDetour>&
  GetDirectInput8CreateDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetDirectInput8Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

void ProxyDirectInput8(REFIID riid, void** direct_input)
{
  HADESMEM_DETAIL_TRACE_A("Proxying IDirectInput8.");

  try
  {
    if (riid == IID_IDirectInput8A)
    {
      HADESMEM_DETAIL_TRACE_A("Proxying IDirectInput8A.");
      *direct_input = new hadesmem::cerberus::DirectInput8AProxy(
        static_cast<IDirectInput8A*>(*direct_input));
    }
    else if (riid == IID_IDirectInput8W)
    {
      HADESMEM_DETAIL_TRACE_A("Proxying IDirectInput8W.");
      *direct_input = new hadesmem::cerberus::DirectInput8WProxy(
        static_cast<IDirectInput8W*>(*direct_input));
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

  ProxyDirectInput8(riid, out);

  return ret;
}
}

namespace hadesmem
{
namespace cerberus
{
DirectInputDeviceType DeviceGuidToEnum(REFGUID rguid)
{
  if (rguid == GUID_SysMouse)
  {
    return DirectInputDeviceType::Mouse;
  }
  else if (rguid == GUID_SysKeyboard)
  {
    return DirectInputDeviceType::Keyboard;
  }
  else
  {
    return DirectInputDeviceType::Other;
  }
}

std::string DeviceGuidToString(REFGUID rguid)
{
  if (rguid == GUID_SysMouse)
  {
    return "Mouse";
  }
  else if (rguid == GUID_SysKeyboard)
  {
    return "Keyboard";
  }
  else
  {
    return "Other";
  }
}

Callbacks<OnDirectInputCallback>& GetOnDirectInputCallbacks()
{
  static Callbacks<OnDirectInputCallback> callbacks;
  return callbacks;
}

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

    module = std::make_pair(nullptr, 0);
  }
}
}
}
