// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include <windows.h>
#include <hadesmem/detail/warning_disable_prefix.hpp>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>

#include "direct_input.hpp"

namespace hadesmem
{
namespace cerberus
{
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif // #if defined(HADESMEM_GCC)

class DirectInputDevice8WProxy : public IDirectInputDevice8W
{
public:
  explicit DirectInputDevice8WProxy(IDirectInputDevice8W* device,
                                    DirectInputDeviceType type)
    : device_{device}, type_{type}
  {
  }

  // IUnknown
  HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* obj) override;
  ULONG WINAPI AddRef() override;
  ULONG WINAPI Release() override;

  // IDirectInputDevice8W
  HRESULT WINAPI GetCapabilities(LPDIDEVCAPS dev_caps) override;
  HRESULT WINAPI EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKW callback,
                             LPVOID ref,
                             DWORD flags) override;
  HRESULT WINAPI
    GetProperty(REFGUID guid_prop, LPDIPROPHEADER prop_header) override;
  HRESULT WINAPI
    SetProperty(REFGUID guid_prop, LPCDIPROPHEADER prop_header) override;
  HRESULT WINAPI Acquire() override;
  HRESULT WINAPI Unacquire() override;
  HRESULT WINAPI GetDeviceState(DWORD len_data, LPVOID data) override;
  HRESULT WINAPI GetDeviceData(DWORD len_object_data,
                               LPDIDEVICEOBJECTDATA rgdod,
                               LPDWORD in_out,
                               DWORD flags) override;
  HRESULT WINAPI SetDataFormat(LPCDIDATAFORMAT data_format) override;
  HRESULT WINAPI SetEventNotification(HANDLE event_handle) override;
  HRESULT WINAPI SetCooperativeLevel(HWND hwnd, DWORD flags) override;
  HRESULT WINAPI GetObjectInfo(LPDIDEVICEOBJECTINSTANCEW obj_instance,
                               DWORD obj,
                               DWORD how) override;
  HRESULT WINAPI GetDeviceInfo(LPDIDEVICEINSTANCEW device_instance) override;
  HRESULT WINAPI RunControlPanel(HWND owner, DWORD flags) override;
  HRESULT WINAPI
    Initialize(HINSTANCE inst, DWORD version, REFGUID guid) override;
  HRESULT WINAPI CreateEffect(REFGUID guid,
                              LPCDIEFFECT effect,
                              LPDIRECTINPUTEFFECT* effect_interface,
                              LPUNKNOWN outer) override;
  HRESULT WINAPI EnumEffects(LPDIENUMEFFECTSCALLBACKW callback,
                             LPVOID ref,
                             DWORD eff_type) override;
  HRESULT WINAPI
    GetEffectInfo(LPDIEFFECTINFOW effect_info, REFGUID guid) override;
  HRESULT WINAPI GetForceFeedbackState(LPDWORD out) override;
  HRESULT WINAPI SendForceFeedbackCommand(DWORD flags) override;
  HRESULT WINAPI
    EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                             LPVOID ref,
                             DWORD flags) override;
  HRESULT WINAPI Escape(LPDIEFFESCAPE escape) override;
  HRESULT WINAPI Poll() override;
  HRESULT WINAPI SendDeviceData(DWORD len_object_data,
                                LPCDIDEVICEOBJECTDATA object_data,
                                LPDWORD in_out,
                                DWORD flags) override;
  HRESULT WINAPI EnumEffectsInFile(LPCWSTR file_name,
                                   LPDIENUMEFFECTSINFILECALLBACK pec,
                                   LPVOID ref,
                                   DWORD flags) override;
  HRESULT WINAPI WriteEffectToFile(LPCWSTR file_name,
                                   DWORD entries,
                                   LPDIFILEEFFECT file_effect,
                                   DWORD flags) override;
  HRESULT WINAPI BuildActionMap(LPDIACTIONFORMATW action_format,
                                LPCWSTR user_name,
                                DWORD flags) override;
  HRESULT WINAPI SetActionMap(LPDIACTIONFORMATW action_format,
                              LPCWSTR user_name,
                              DWORD flags) override;
  HRESULT WINAPI
    GetImageInfo(LPDIDEVICEIMAGEINFOHEADERW dev_image_info_header) override;

private:
  void Cleanup();

  std::int64_t refs_{1};
  IDirectInputDevice8W* device_{};
  DirectInputDeviceType type_{DirectInputDeviceType::Other};
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)
}
}
