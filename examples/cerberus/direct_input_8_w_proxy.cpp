// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "direct_input_8_w_proxy.hpp"

#include <string>

#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>

#include "direct_input.hpp"
#include "direct_input_device_8_w_proxy.hpp"

namespace hadesmem
{
namespace cerberus
{
HRESULT WINAPI DirectInput8WProxy::QueryInterface(REFIID riid, LPVOID* obj)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  last_error_preserver.Revert();
  auto const ret = direct_input_->QueryInterface(riid, obj);
  last_error_preserver.Update();

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (*obj == direct_input_)
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

ULONG WINAPI DirectInput8WProxy::AddRef()
{
  refs_++;
  auto const ret = direct_input_->AddRef();
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);
  return ret;
}

ULONG WINAPI DirectInput8WProxy::Release()
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  refs_--;
  HADESMEM_DETAIL_ASSERT(refs_ >= 0);

  if (refs_ == 0)
  {
    Cleanup();
  }

  last_error_preserver.Revert();
  auto const ret = direct_input_->Release();
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);

  if (ret == 0)
  {
    delete this;
  }

  return ret;
}

HRESULT WINAPI DirectInput8WProxy::CreateDevice(REFGUID rguid,
                                                LPDIRECTINPUTDEVICE8W* device,
                                                LPUNKNOWN outer)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%p] [%p] [%p].", this, &rguid, device, outer);

  last_error_preserver.Revert();
  auto const ret = direct_input_->CreateDevice(rguid, device, outer);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("Got new DirectInputDevice8W. Type: [%s].",
                                   DeviceGuidToString(rguid).c_str());
    *device = new DirectInputDevice8WProxy(*device, DeviceGuidToEnum(rguid));
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

HRESULT WINAPI DirectInput8WProxy::EnumDevices(
  DWORD dev_type, LPDIENUMDEVICESCALLBACKW callback, LPVOID ref, DWORD flags)
{
  return direct_input_->EnumDevices(dev_type, callback, ref, flags);
}

HRESULT WINAPI DirectInput8WProxy::GetDeviceStatus(REFGUID rguid)
{
  return direct_input_->GetDeviceStatus(rguid);
}

HRESULT WINAPI DirectInput8WProxy::RunControlPanel(HWND owner, DWORD flags)
{
  return direct_input_->RunControlPanel(owner, flags);
}

HRESULT WINAPI DirectInput8WProxy::Initialize(HINSTANCE hinst, DWORD version)
{
  return direct_input_->Initialize(hinst, version);
}

HRESULT WINAPI DirectInput8WProxy::FindDevice(REFGUID rguid,
                                              LPCWSTR name,
                                              LPGUID guid_instance)
{
  return direct_input_->FindDevice(rguid, name, guid_instance);
}

HRESULT WINAPI DirectInput8WProxy::EnumDevicesBySemantics(
  LPCWSTR user_name,
  LPDIACTIONFORMATW action_format,
  LPDIENUMDEVICESBYSEMANTICSCBW callback,
  LPVOID ref,
  DWORD flags)
{
  return direct_input_->EnumDevicesBySemantics(
    user_name, action_format, callback, ref, flags);
}

HRESULT WINAPI
  DirectInput8WProxy::ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK callback,
                                       LPDICONFIGUREDEVICESPARAMSW cd_params,
                                       DWORD flags,
                                       LPVOID ref_data)
{
  return direct_input_->ConfigureDevices(callback, cd_params, flags, ref_data);
}

void DirectInput8WProxy::Cleanup()
{
  HADESMEM_DETAIL_TRACE_A("Called.");
}
}
}
