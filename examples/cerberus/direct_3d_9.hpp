// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>

#include <windows.h>

#include <d3d9.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif // #if defined(HADESMEM_GCC)

class Direct3D9Proxy : public IDirect3D9 /*: public IDirect3D9Ex*/
{
public:
  explicit Direct3D9Proxy(IDirect3D9* d3d9)
    : d3d9_{static_cast<IDirect3D9Ex*>(d3d9)}
  {
  }

  // IUnknown
  virtual HRESULT WINAPI QueryInterface(REFIID riid, void** obj) override;
  virtual ULONG WINAPI AddRef() override;
  virtual ULONG WINAPI Release() override;

  // IDirect3D9
  virtual HRESULT WINAPI
    RegisterSoftwareDevice(void* initialize_function) override;
  virtual UINT WINAPI GetAdapterCount() override;
  virtual HRESULT WINAPI
    GetAdapterIdentifier(UINT adapter,
                         DWORD flags,
                         D3DADAPTER_IDENTIFIER9* identifier) override;
  virtual UINT WINAPI
    GetAdapterModeCount(UINT adapter, D3DFORMAT format) override;
  virtual HRESULT WINAPI EnumAdapterModes(UINT adapter,
                                          D3DFORMAT format,
                                          UINT mode_index,
                                          D3DDISPLAYMODE* mode) override;
  virtual HRESULT WINAPI
    GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE* mode) override;
  virtual HRESULT WINAPI CheckDeviceType(UINT adapter,
                                         D3DDEVTYPE dev_type,
                                         D3DFORMAT adapter_format,
                                         D3DFORMAT back_buffer_format,
                                         BOOL windowed) override;
  virtual HRESULT WINAPI CheckDeviceFormat(UINT adapter,
                                           D3DDEVTYPE device_type,
                                           D3DFORMAT adapter_format,
                                           DWORD usage,
                                           D3DRESOURCETYPE type,
                                           D3DFORMAT check_format) override;
  virtual HRESULT WINAPI
    CheckDeviceMultiSampleType(UINT adapter,
                               D3DDEVTYPE device_type,
                               D3DFORMAT surface_format,
                               BOOL windowed,
                               D3DMULTISAMPLE_TYPE multi_sample_type,
                               DWORD* quality_levels) override;
  virtual HRESULT WINAPI
    CheckDepthStencilMatch(UINT adapter,
                           D3DDEVTYPE device_type,
                           D3DFORMAT adapter_format,
                           D3DFORMAT render_target_format,
                           D3DFORMAT depth_stencil_format) override;
  virtual HRESULT WINAPI
    CheckDeviceFormatConversion(UINT adapter,
                                D3DDEVTYPE device_type,
                                D3DFORMAT source_format,
                                D3DFORMAT target_format) override;
  virtual HRESULT WINAPI GetDeviceCaps(UINT adapter,
                                       D3DDEVTYPE device_type,
                                       D3DCAPS9* caps) override;
  virtual HMONITOR WINAPI GetAdapterMonitor(UINT adapter) override;
  virtual HRESULT WINAPI
    CreateDevice(UINT adapter,
                 D3DDEVTYPE device_type,
                 HWND focus_window,
                 DWORD behaviour_flags,
                 D3DPRESENT_PARAMETERS* presentation_parameters,
                 IDirect3DDevice9** returned_device_interface) override;

  // IDirect3D9Ex
  virtual UINT WINAPI
    GetAdapterModeCountEx(UINT adapter,
                          CONST D3DDISPLAYMODEFILTER* filter) /*override*/;
  virtual HRESULT WINAPI
    EnumAdapterModesEx(UINT adapter,
                       CONST D3DDISPLAYMODEFILTER* filter,
                       UINT mode_index,
                       D3DDISPLAYMODEEX* mode) /*override*/;
  virtual HRESULT WINAPI
    GetAdapterDisplayModeEx(UINT adapter,
                            D3DDISPLAYMODEEX* mode,
                            D3DDISPLAYROTATION* rotation) /*override*/;
  virtual HRESULT WINAPI
    CreateDeviceEx(UINT adapter,
                   D3DDEVTYPE device_type,
                   HWND focus_window,
                   DWORD behavior_flags,
                   D3DPRESENT_PARAMETERS* presentation_parameters,
                   D3DDISPLAYMODEEX* fullscreen_display_mode,
                   IDirect3DDevice9Ex** returned_device_interface) /*override*/;
  virtual HRESULT WINAPI GetAdapterLUID(UINT adapter, LUID* luid) /*override*/;

protected:
  void Cleanup();

  std::int64_t refs_{1};
  IDirect3D9Ex* d3d9_{};
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)
}
}
