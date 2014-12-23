// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "direct_3d_9.hpp"

#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>

#include "direct_3d_device_9.hpp"

namespace hadesmem
{
namespace cerberus
{
HRESULT WINAPI Direct3D9Proxy::QueryInterface(REFIID riid, void** obj)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  last_error_preserver.Revert();
  auto const ret = d3d9_->QueryInterface(riid, obj);
  last_error_preserver.Update();

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (*obj == d3d9_)
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

ULONG WINAPI Direct3D9Proxy::AddRef()
{
  refs_++;
  auto const ret = d3d9_->AddRef();
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);
  return ret;
}

ULONG WINAPI Direct3D9Proxy::Release()
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  refs_--;
  HADESMEM_DETAIL_ASSERT(refs_ >= 0);

  if (refs_ == 0)
  {
    Cleanup();
  }

  last_error_preserver.Revert();
  auto const ret = d3d9_->Release();
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);

  if (ret == 0)
  {
    delete this;
  }

  return ret;
}

HRESULT WINAPI Direct3D9Proxy::RegisterSoftwareDevice(void* initialize_function)
{
  return d3d9_->RegisterSoftwareDevice(initialize_function);
}

UINT WINAPI Direct3D9Proxy::GetAdapterCount()
{
  return d3d9_->GetAdapterCount();
}

HRESULT WINAPI
  Direct3D9Proxy::GetAdapterIdentifier(UINT adapter,
                                       DWORD flags,
                                       D3DADAPTER_IDENTIFIER9* identifier)
{
  return d3d9_->GetAdapterIdentifier(adapter, flags, identifier);
}

UINT WINAPI Direct3D9Proxy::GetAdapterModeCount(UINT adapter, D3DFORMAT format)
{
  return d3d9_->GetAdapterModeCount(adapter, format);
}

HRESULT WINAPI Direct3D9Proxy::EnumAdapterModes(UINT adapter,
                                                D3DFORMAT format,
                                                UINT mode_index,
                                                D3DDISPLAYMODE* mode)
{
  return d3d9_->EnumAdapterModes(adapter, format, mode_index, mode);
}

HRESULT WINAPI
  Direct3D9Proxy::GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE* mode)
{
  return d3d9_->GetAdapterDisplayMode(adapter, mode);
}

HRESULT WINAPI Direct3D9Proxy::CheckDeviceType(UINT adapter,
                                               D3DDEVTYPE dev_type,
                                               D3DFORMAT adapter_format,
                                               D3DFORMAT back_buffer_format,
                                               BOOL windowed)
{
  return d3d9_->CheckDeviceType(
    adapter, dev_type, adapter_format, back_buffer_format, windowed);
}

HRESULT WINAPI Direct3D9Proxy::CheckDeviceFormat(UINT adapter,
                                                 D3DDEVTYPE device_type,
                                                 D3DFORMAT adapter_format,
                                                 DWORD usage,
                                                 D3DRESOURCETYPE type,
                                                 D3DFORMAT check_format)
{
  return d3d9_->CheckDeviceFormat(
    adapter, device_type, adapter_format, usage, type, check_format);
}

HRESULT WINAPI Direct3D9Proxy::CheckDeviceMultiSampleType(
  UINT adapter,
  D3DDEVTYPE device_type,
  D3DFORMAT surface_format,
  BOOL windowed,
  D3DMULTISAMPLE_TYPE multi_sample_type,
  DWORD* quality_levels)
{
  return d3d9_->CheckDeviceMultiSampleType(adapter,
                                           device_type,
                                           surface_format,
                                           windowed,
                                           multi_sample_type,
                                           quality_levels);
}

HRESULT WINAPI
  Direct3D9Proxy::CheckDepthStencilMatch(UINT adapter,
                                         D3DDEVTYPE device_type,
                                         D3DFORMAT adapter_format,
                                         D3DFORMAT render_target_format,
                                         D3DFORMAT depth_stencil_format)
{
  return d3d9_->CheckDepthStencilMatch(adapter,
                                       device_type,
                                       adapter_format,
                                       render_target_format,
                                       depth_stencil_format);
}

HRESULT WINAPI
  Direct3D9Proxy::CheckDeviceFormatConversion(UINT adapter,
                                              D3DDEVTYPE device_type,
                                              D3DFORMAT source_format,
                                              D3DFORMAT target_format)
{
  return d3d9_->CheckDeviceFormatConversion(
    adapter, device_type, source_format, target_format);
}

HRESULT WINAPI Direct3D9Proxy::GetDeviceCaps(UINT adapter,
                                             D3DDEVTYPE device_type,
                                             D3DCAPS9* caps)
{
  return d3d9_->GetDeviceCaps(adapter, device_type, caps);
}

HMONITOR WINAPI Direct3D9Proxy::GetAdapterMonitor(UINT adapter)
{
  return d3d9_->GetAdapterMonitor(adapter);
}

HRESULT WINAPI
  Direct3D9Proxy::CreateDevice(UINT adapter,
                               D3DDEVTYPE device_type,
                               HWND focus_window,
                               DWORD behaviour_flags,
                               D3DPRESENT_PARAMETERS* presentation_parameters,
                               IDirect3DDevice9** returned_device_interface)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%u] [%d] [%p] [%u] [%p] [%p].",
                                 this,
                                 adapter,
                                 device_type,
                                 focus_window,
                                 behaviour_flags,
                                 presentation_parameters,
                                 returned_device_interface);

  last_error_preserver.Revert();
  auto const ret = d3d9_->CreateDevice(adapter,
                                       device_type,
                                       focus_window,
                                       behaviour_flags,
                                       presentation_parameters,
                                       returned_device_interface);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (returned_device_interface)
    {
      HADESMEM_DETAIL_TRACE_A("Proxying IDirect3DDevice9.");
      *returned_device_interface =
        new Direct3DDevice9Proxy{*returned_device_interface};
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid device out param pointer.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

UINT WINAPI
  Direct3D9Proxy::GetAdapterModeCountEx(UINT adapter,
                                        CONST D3DDISPLAYMODEFILTER* filter)
{
  return d3d9_->GetAdapterModeCountEx(adapter, filter);
}

HRESULT WINAPI
  Direct3D9Proxy::EnumAdapterModesEx(UINT adapter,
                                     CONST D3DDISPLAYMODEFILTER* filter,
                                     UINT mode_index,
                                     D3DDISPLAYMODEEX* mode)
{
  return d3d9_->EnumAdapterModesEx(adapter, filter, mode_index, mode);
}

HRESULT WINAPI
  Direct3D9Proxy::GetAdapterDisplayModeEx(UINT adapter,
                                          D3DDISPLAYMODEEX* mode,
                                          D3DDISPLAYROTATION* rotation)
{
  return d3d9_->GetAdapterDisplayModeEx(adapter, mode, rotation);
}

HRESULT WINAPI
  Direct3D9Proxy::CreateDeviceEx(UINT adapter,
                                 D3DDEVTYPE device_type,
                                 HWND focus_window,
                                 DWORD behavior_flags,
                                 D3DPRESENT_PARAMETERS* presentation_parameters,
                                 D3DDISPLAYMODEEX* fullscreen_display_mode,
                                 IDirect3DDevice9Ex** returned_device_interface)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%u] [%d] [%p] [%u] [%p] [%p].",
                                 this,
                                 adapter,
                                 device_type,
                                 focus_window,
                                 behavior_flags,
                                 presentation_parameters,
                                 returned_device_interface);

  last_error_preserver.Revert();
  auto const ret = d3d9_->CreateDeviceEx(adapter,
                                         device_type,
                                         focus_window,
                                         behavior_flags,
                                         presentation_parameters,
                                         fullscreen_display_mode,
                                         returned_device_interface);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (returned_device_interface)
    {
      HADESMEM_DETAIL_TRACE_A("Proxying IDirect3DDevice9Ex.");
      *returned_device_interface =
        new Direct3DDevice9Proxy{*returned_device_interface};
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid device out param pointer.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

HRESULT WINAPI Direct3D9Proxy::GetAdapterLUID(UINT adapter, LUID* luid)
{
  return d3d9_->GetAdapterLUID(adapter, luid);
}

void Direct3D9Proxy::Cleanup()
{
  HADESMEM_DETAIL_TRACE_A("Called.");
}
}
}
