// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "direct_input_device_8_a_proxy.hpp"

#include <cstring>

#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>

#include "direct_input.hpp"

namespace hadesmem
{
namespace cerberus
{
HRESULT WINAPI
  DirectInputDevice8AProxy::QueryInterface(REFIID riid, LPVOID* obj)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  last_error_preserver.Revert();
  auto const ret = device_->QueryInterface(riid, obj);
  last_error_preserver.Update();

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (*obj == device_)
    {
      refs_++;
      *obj = this;
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("WARNING! Unhandled interface.");
      HADESMEM_DETAIL_ASSERT(false);
      static_cast<IUnknown*>(*obj)->Release();
      return E_NOINTERFACE;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

ULONG WINAPI DirectInputDevice8AProxy::AddRef()
{
  refs_++;
  auto const ret = device_->AddRef();
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);
  return ret;
}

ULONG WINAPI DirectInputDevice8AProxy::Release()
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  refs_--;
  HADESMEM_DETAIL_ASSERT(refs_ >= 0);

  if (refs_ == 0)
  {
    Cleanup();
  }

  last_error_preserver.Revert();
  auto const ret = device_->Release();
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);

  if (ret == 0)
  {
    delete this;
  }

  return ret;
}

HRESULT WINAPI DirectInputDevice8AProxy::GetCapabilities(LPDIDEVCAPS dev_caps)
{
  return device_->GetCapabilities(dev_caps);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA callback,
                                        LPVOID ref,
                                        DWORD flags)
{
  return device_->EnumObjects(callback, ref, flags);
}

HRESULT WINAPI DirectInputDevice8AProxy::GetProperty(REFGUID guid_prop,
                                                     LPDIPROPHEADER prop_header)
{
  return device_->GetProperty(guid_prop, prop_header);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::SetProperty(REFGUID guid_prop,
                                        LPCDIPROPHEADER prop_header)
{
  return device_->SetProperty(guid_prop, prop_header);
}

HRESULT WINAPI DirectInputDevice8AProxy::Acquire()
{
  return device_->Acquire();
}

HRESULT WINAPI DirectInputDevice8AProxy::Unacquire()
{
  return device_->Unacquire();
}

HRESULT WINAPI
  DirectInputDevice8AProxy::GetDeviceState(DWORD len_data, LPVOID data)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  last_error_preserver.Revert();
  auto const ret = device_->GetDeviceState(len_data, data);
  last_error_preserver.Update();

  auto const& callbacks = GetOnDirectInputCallbacks();
  bool handled = false;
  callbacks.Run(&handled);

  if (handled && SUCCEEDED(ret))
  {
    std::memset(data, 0, len_data);
    return E_FAIL;
  }

  return ret;
}

HRESULT WINAPI
  DirectInputDevice8AProxy::GetDeviceData(DWORD len_object_data,
                                          LPDIDEVICEOBJECTDATA rgdod,
                                          LPDWORD in_out,
                                          DWORD flags)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  last_error_preserver.Revert();
  auto const ret =
    device_->GetDeviceData(len_object_data, rgdod, in_out, flags);
  last_error_preserver.Update();

  auto const& callbacks = GetOnDirectInputCallbacks();
  bool handled = false;
  callbacks.Run(&handled);

  if (handled && SUCCEEDED(ret))
  {
    // Flush direct input buffer
    DWORD items = INFINITE;
    device_->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), nullptr, &items, 0);

    // Zero application buffer
    if (rgdod)
    {
      std::memset(rgdod, 0, len_object_data * *in_out);
    }

    return E_FAIL;
  }

  return ret;
}

HRESULT WINAPI
  DirectInputDevice8AProxy::SetDataFormat(LPCDIDATAFORMAT data_format)
{
  return device_->SetDataFormat(data_format);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::SetEventNotification(HANDLE event_handle)
{
  return device_->SetEventNotification(event_handle);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::SetCooperativeLevel(HWND hwnd, DWORD flags)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%lu]", hwnd, flags);
  return device_->SetCooperativeLevel(hwnd, flags);
}

HRESULT WINAPI DirectInputDevice8AProxy::GetObjectInfo(
  LPDIDEVICEOBJECTINSTANCEA obj_instance, DWORD obj, DWORD how)
{
  return device_->GetObjectInfo(obj_instance, obj, how);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::GetDeviceInfo(LPDIDEVICEINSTANCEA device_instance)
{
  return device_->GetDeviceInfo(device_instance);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::RunControlPanel(HWND owner, DWORD flags)
{
  return device_->RunControlPanel(owner, flags);
}

HRESULT WINAPI DirectInputDevice8AProxy::Initialize(HINSTANCE inst,
                                                    DWORD version,
                                                    REFGUID guid)
{
  return device_->Initialize(inst, version, guid);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::CreateEffect(REFGUID guid,
                                         LPCDIEFFECT effect,
                                         LPDIRECTINPUTEFFECT* effect_interface,
                                         LPUNKNOWN outer)
{
  return device_->CreateEffect(guid, effect, effect_interface, outer);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::EnumEffects(LPDIENUMEFFECTSCALLBACKA callback,
                                        LPVOID ref,
                                        DWORD eff_type)
{
  return device_->EnumEffects(callback, ref, eff_type);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::GetEffectInfo(LPDIEFFECTINFOA effect_info,
                                          REFGUID guid)
{
  return device_->GetEffectInfo(effect_info, guid);
}

HRESULT WINAPI DirectInputDevice8AProxy::GetForceFeedbackState(LPDWORD out)
{
  return device_->GetForceFeedbackState(out);
}

HRESULT WINAPI DirectInputDevice8AProxy::SendForceFeedbackCommand(DWORD flags)
{
  return device_->SendForceFeedbackCommand(flags);
}

HRESULT WINAPI DirectInputDevice8AProxy::EnumCreatedEffectObjects(
  LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback, LPVOID ref, DWORD flags)
{
  return device_->EnumCreatedEffectObjects(callback, ref, flags);
}

HRESULT WINAPI DirectInputDevice8AProxy::Escape(LPDIEFFESCAPE escape)
{
  return device_->Escape(escape);
}

HRESULT WINAPI DirectInputDevice8AProxy::Poll()
{
  return device_->Poll();
}

HRESULT WINAPI
  DirectInputDevice8AProxy::SendDeviceData(DWORD len_object_data,
                                           LPCDIDEVICEOBJECTDATA object_data,
                                           LPDWORD in_out,
                                           DWORD flags)
{
  return device_->SendDeviceData(len_object_data, object_data, in_out, flags);
}

HRESULT WINAPI DirectInputDevice8AProxy::EnumEffectsInFile(
  LPCSTR file_name, LPDIENUMEFFECTSINFILECALLBACK pec, LPVOID ref, DWORD flags)
{
  return device_->EnumEffectsInFile(file_name, pec, ref, flags);
}

HRESULT WINAPI DirectInputDevice8AProxy::WriteEffectToFile(
  LPCSTR file_name, DWORD entries, LPDIFILEEFFECT file_effect, DWORD flags)
{
  return device_->WriteEffectToFile(file_name, entries, file_effect, flags);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::BuildActionMap(LPDIACTIONFORMATA action_format,
                                           LPCSTR user_name,
                                           DWORD flags)
{
  return device_->BuildActionMap(action_format, user_name, flags);
}

HRESULT WINAPI
  DirectInputDevice8AProxy::SetActionMap(LPDIACTIONFORMATA action_format,
                                         LPCSTR user_name,
                                         DWORD flags)
{
  return device_->SetActionMap(action_format, user_name, flags);
}

HRESULT WINAPI DirectInputDevice8AProxy::GetImageInfo(
  LPDIDEVICEIMAGEINFOHEADERA dev_image_info_header)
{
  return device_->GetImageInfo(dev_image_info_header);
}

void DirectInputDevice8AProxy::Cleanup()
{
  HADESMEM_DETAIL_TRACE_A("Called.");
}
}
}
