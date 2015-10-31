// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "direct_input.hpp"

#include <cstdint>

#include <windows.h>
#include <hadesmem/detail/warning_disable_prefix.hpp>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/patcher.hpp>

#include "callbacks.hpp"
#include "direct_input_8_a_proxy.hpp"
#include "direct_input_8_w_proxy.hpp"
#include "helpers.hpp"
#include "main.hpp"

// TODO: Replace device proxies with inline hooks like what was done for D3D.

namespace
{
class DirectInputImpl : public hadesmem::cerberus::DirectInputInterface
{
public:
  virtual std::size_t RegisterOnGetDeviceData(
    std::function<hadesmem::cerberus::OnGetDeviceDataCallback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnGetDeviceDataCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnGetDeviceData(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnGetDeviceDataCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnGetDeviceState(
    std::function<hadesmem::cerberus::OnGetDeviceStateCallback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnGetDeviceStateCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnGetDeviceState(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnGetDeviceStateCallbacks();
    return callbacks.Unregister(id);
  }
};

std::unique_ptr<hadesmem::PatchDetour<decltype(&::DirectInput8Create)>>&
  GetDirectInput8CreateDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::DirectInput8Create)>>
    detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetDirectInput8Module() noexcept
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
  DirectInput8CreateDetour(hadesmem::PatchDetourBase* detour,
                           HINSTANCE hinst,
                           DWORD version,
                           REFIID riid,
                           LPVOID* out,
                           LPUNKNOWN unk_outer) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%u] [%p] [%p] [%p].", hinst, version, &riid, out, unk_outer);

  auto const direct_input_8_create =
    detour->GetTrampolineT<decltype(&DirectInput8Create)>();
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

struct DeviceEnumInfo
{
  GUID guid_{};
  hadesmem::cerberus::DirectInputDeviceType type_{
    hadesmem::cerberus::DirectInputDeviceType::Other};
};
}

namespace hadesmem
{
namespace cerberus
{
DirectInputDeviceType DeviceGuidToEnum(IDirectInput8A* direct_input,
                                       REFGUID rguid)
{
  auto enum_callback = [](LPCDIDEVICEINSTANCEA device_instance,
                          LPVOID ref) -> BOOL
  {
    auto const enum_info = static_cast<DeviceEnumInfo*>(ref);
    if (device_instance->guidInstance == enum_info->guid_ ||
        device_instance->guidProduct == enum_info->guid_)
    {
      auto const type = GET_DIDEVICE_TYPE(device_instance->dwDevType);
      switch (type)
      {
      case DI8DEVTYPE_KEYBOARD:
        enum_info->type_ = DirectInputDeviceType::Mouse;
        break;
      case DI8DEVTYPE_MOUSE:
        enum_info->type_ = DirectInputDeviceType::Keyboard;
        break;
      default:
        enum_info->type_ = DirectInputDeviceType::Other;
        break;
      }

      auto const& rguid = enum_info->guid_;
      (void)rguid;
      HADESMEM_DETAIL_TRACE_FORMAT_A("Got match. Type: [%lu]. GUID: "
                                     "[%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%"
                                     "02hhX%02hhX%02hhX%02hhX%02hhX]",
                                     type,
                                     rguid.Data1,
                                     rguid.Data2,
                                     rguid.Data3,
                                     rguid.Data4[0],
                                     rguid.Data4[1],
                                     rguid.Data4[2],
                                     rguid.Data4[3],
                                     rguid.Data4[4],
                                     rguid.Data4[5],
                                     rguid.Data4[6],
                                     rguid.Data4[7]);

      return DIENUM_STOP;
    }

    HADESMEM_DETAIL_TRACE_FORMAT_A("No match for current GUID.");

    return DIENUM_CONTINUE;
  };

  DeviceEnumInfo device_enum_info;
  device_enum_info.guid_ = rguid;
  auto const hr = direct_input->EnumDevices(
    DI8DEVCLASS_ALL, enum_callback, &device_enum_info, DIEDFL_ALLDEVICES);
  if (FAILED(hr))
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "EnumDevices failed. Defaulting to 'Other' device type. HR: [%08X]", hr);
    return DirectInputDeviceType::Other;
  }

  return device_enum_info.type_;
}

DirectInputDeviceType DeviceGuidToEnum(IDirectInput8W* direct_input,
                                       REFGUID rguid)
{
  auto enum_callback = [](LPCDIDEVICEINSTANCEW device_instance,
                          LPVOID ref) -> BOOL
  {
    auto const enum_info = static_cast<DeviceEnumInfo*>(ref);
    if (device_instance->guidInstance == enum_info->guid_ ||
        device_instance->guidProduct == enum_info->guid_)
    {
      auto const type = GET_DIDEVICE_TYPE(device_instance->dwDevType);
      switch (type)
      {
      case DI8DEVTYPE_KEYBOARD:
        enum_info->type_ = DirectInputDeviceType::Mouse;
        break;
      case DI8DEVTYPE_MOUSE:
        enum_info->type_ = DirectInputDeviceType::Keyboard;
        break;
      default:
        enum_info->type_ = DirectInputDeviceType::Other;
        break;
      }

      auto const& rguid = enum_info->guid_;
      (void)rguid;
      HADESMEM_DETAIL_TRACE_FORMAT_A("Got match. Type: [%lu]. GUID: "
                                     "[%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%"
                                     "02hhX%02hhX%02hhX%02hhX%02hhX]",
                                     type,
                                     rguid.Data1,
                                     rguid.Data2,
                                     rguid.Data3,
                                     rguid.Data4[0],
                                     rguid.Data4[1],
                                     rguid.Data4[2],
                                     rguid.Data4[3],
                                     rguid.Data4[4],
                                     rguid.Data4[5],
                                     rguid.Data4[6],
                                     rguid.Data4[7]);

      return DIENUM_STOP;
    }

    HADESMEM_DETAIL_TRACE_FORMAT_A("No match for current GUID.");

    return DIENUM_CONTINUE;
  };

  DeviceEnumInfo device_enum_info;
  device_enum_info.guid_ = rguid;
  auto const hr = direct_input->EnumDevices(
    DI8DEVCLASS_ALL, enum_callback, &device_enum_info, DIEDFL_ALLDEVICES);
  if (FAILED(hr))
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "EnumDevices failed. Defaulting to 'Other' device type. HR: [%08X]", hr);
    return DirectInputDeviceType::Other;
  }

  return device_enum_info.type_;
}

std::string DeviceTypeToString(DirectInputDeviceType type)
{
  if (type == DirectInputDeviceType::Mouse)
  {
    return "Mouse";
  }
  else if (type == DirectInputDeviceType::Keyboard)
  {
    return "Keyboard";
  }
  else
  {
    return "Other";
  }
}

Callbacks<OnGetDeviceDataCallback>& GetOnGetDeviceDataCallbacks()
{
  static Callbacks<OnGetDeviceDataCallback> callbacks;
  return callbacks;
}

Callbacks<OnGetDeviceStateCallback>& GetOnGetDeviceStateCallbacks()
{
  static Callbacks<OnGetDeviceStateCallback> callbacks;
  return callbacks;
}

DirectInputInterface& GetDirectInputInterface() noexcept
{
  static DirectInputImpl input_impl;
  return input_impl;
}

void InitializeDirectInput()
{
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(L"DINPUT8",
                                    DetourDirectInput8,
                                    UndetourDirectInput8,
                                    GetDirectInput8Module);
}

void DetourDirectInput8(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetDirectInput8Module();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"dinput8", base, module))
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
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"dinput8", module))
  {
    UndetourFunc(L"DirectInput8Create", GetDirectInput8CreateDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}
}
}
