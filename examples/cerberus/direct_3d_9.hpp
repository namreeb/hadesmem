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
class Direct3D9Proxy : public IDirect3D9Ex
{
public:
  explicit Direct3D9Proxy(IDirect3D9* d3d9) : d3d9_{d3d9}
  {
  }

  // IUnknown
  HRESULT WINAPI QueryInterface(REFIID riid, void** obj) override;
  ULONG WINAPI AddRef() override;
  ULONG WINAPI Release() override;

  // IDirect3D9
  HRESULT WINAPI RegisterSoftwareDevice(void* initialize_function) override;
  UINT WINAPI GetAdapterCount() override;
  HRESULT WINAPI
    GetAdapterIdentifier(UINT adapter,
                         DWORD flags,
                         D3DADAPTER_IDENTIFIER9* identifier) override;
  UINT WINAPI GetAdapterModeCount(UINT adapter, D3DFORMAT format) override;
  HRESULT WINAPI EnumAdapterModes(UINT adapter,
                                  D3DFORMAT format,
                                  UINT mode_index,
                                  D3DDISPLAYMODE* mode) override;
  HRESULT WINAPI
    GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE* mode) override;
  HRESULT WINAPI CheckDeviceType(UINT adapter,
                                 D3DDEVTYPE dev_type,
                                 D3DFORMAT adapter_format,
                                 D3DFORMAT back_buffer_format,
                                 BOOL windowed) override;
  HRESULT WINAPI CheckDeviceFormat(UINT adapter,
                                   D3DDEVTYPE device_type,
                                   D3DFORMAT adapter_format,
                                   DWORD usage,
                                   D3DRESOURCETYPE type,
                                   D3DFORMAT check_format) override;
  HRESULT WINAPI
    CheckDeviceMultiSampleType(UINT adapter,
                               D3DDEVTYPE device_type,
                               D3DFORMAT surface_format,
                               BOOL windowed,
                               D3DMULTISAMPLE_TYPE multi_sample_type,
                               DWORD* quality_levels) override;
  HRESULT WINAPI
    CheckDepthStencilMatch(UINT adapter,
                           D3DDEVTYPE device_type,
                           D3DFORMAT adapter_format,
                           D3DFORMAT render_target_format,
                           D3DFORMAT depth_stencil_format) override;
  HRESULT WINAPI CheckDeviceFormatConversion(UINT adapter,
                                             D3DDEVTYPE device_type,
                                             D3DFORMAT source_format,
                                             D3DFORMAT target_format) override;
  HRESULT WINAPI GetDeviceCaps(UINT adapter,
                               D3DDEVTYPE device_type,
                               D3DCAPS9* caps) override;
  HMONITOR WINAPI GetAdapterMonitor(UINT adapter) override;
  HRESULT WINAPI
    CreateDevice(UINT adapter,
                 D3DDEVTYPE device_type,
                 HWND focus_window,
                 DWORD behaviour_flags,
                 D3DPRESENT_PARAMETERS* presentation_parameters,
                 IDirect3DDevice9** returned_device_interface) override;

  // IDirect3D9Ex
  UINT WINAPI
    GetAdapterModeCountEx(UINT adapter,
                          CONST D3DDISPLAYMODEFILTER* filter) override;
  HRESULT WINAPI EnumAdapterModesEx(UINT adapter,
                                    CONST D3DDISPLAYMODEFILTER* filter,
                                    UINT mode_index,
                                    D3DDISPLAYMODEEX* mode) override;
  HRESULT WINAPI GetAdapterDisplayModeEx(UINT adapter,
                                         D3DDISPLAYMODEEX* mode,
                                         D3DDISPLAYROTATION* rotation) override;
  HRESULT WINAPI
    CreateDeviceEx(UINT adapter,
                   D3DDEVTYPE device_type,
                   HWND focus_window,
                   DWORD behavior_flags,
                   D3DPRESENT_PARAMETERS* presentation_parameters,
                   D3DDISPLAYMODEEX* fullscreen_display_mode,
                   IDirect3DDevice9Ex** returned_device_interface) override;
  HRESULT WINAPI GetAdapterLUID(UINT adapter, LUID* luid) override;

protected:
  void Cleanup();

  std::int64_t refs_{1};
  IDirect3D9* d3d9_{};
};
}
}
