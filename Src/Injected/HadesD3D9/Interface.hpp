/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// Windows API
#include <Windows.h>

// DirectX
#include <d3d9.h>
#include <d3dx9.h>

// Hades
#include "HadesCommon/Error.hpp"

namespace Hades
{
  namespace D3D9
  {
    class IDirect3D9Hook : public IDirect3D9
    {
    public:
      class Error : public virtual HadesError 
      { };

      explicit IDirect3D9Hook(IDirect3D9* pD3D9);
      
      // IUnknown methods
      STDMETHOD(QueryInterface)(
        THIS_ REFIID riid, 
        void** ppvObj);
      
      STDMETHOD_(ULONG, AddRef)(
        THIS);
      
      STDMETHOD_(ULONG, Release)(
        THIS);
      
      // IDirect3D9 methods
      STDMETHOD(RegisterSoftwareDevice)(
        THIS_ void* pInitializeFunction);
       
      STDMETHOD_(UINT, GetAdapterCount)(
        THIS);
      
      STDMETHOD(GetAdapterIdentifier)(
        THIS_ UINT Adapter, 
        DWORD Flags, 
        D3DADAPTER_IDENTIFIER9* pIdentifier);
      
      STDMETHOD_(UINT, GetAdapterModeCount)(
        THIS_ UINT Adapter, 
        D3DFORMAT Format);
      
      STDMETHOD(EnumAdapterModes)(
        THIS_ UINT Adapter, 
        D3DFORMAT Format, 
        UINT Mode, 
        D3DDISPLAYMODE* pMode);
      
      STDMETHOD(GetAdapterDisplayMode)(
        THIS_ UINT Adapter, 
        D3DDISPLAYMODE* pMode);
      
      STDMETHOD(CheckDeviceType)(
        THIS_ UINT Adapter, 
        D3DDEVTYPE DevType, 
        D3DFORMAT AdapterFormat, 
        D3DFORMAT BackBufferFormat, 
        BOOL bWindowed);
      
      STDMETHOD(CheckDeviceFormat)(
        THIS_ UINT Adapter, 
        D3DDEVTYPE DeviceType, 
        D3DFORMAT AdapterFormat, DWORD Usage, 
        D3DRESOURCETYPE RType, 
        D3DFORMAT CheckFormat);
      
      STDMETHOD(CheckDeviceMultiSampleType)(
        THIS_ UINT Adapter, 
        D3DDEVTYPE DeviceType, 
        D3DFORMAT SurfaceFormat, 
        BOOL Windowed, 
        D3DMULTISAMPLE_TYPE MultiSampleType, 
        DWORD* pQualityLevels);
      
      STDMETHOD(CheckDepthStencilMatch)(
        THIS_ UINT Adapter, 
        D3DDEVTYPE DeviceType, 
        D3DFORMAT AdapterFormat, 
        D3DFORMAT RenderTargetFormat, 
        D3DFORMAT DepthStencilFormat);
      
      STDMETHOD(CheckDeviceFormatConversion)(
        THIS_ UINT Adapter, 
        D3DDEVTYPE DeviceType, 
        D3DFORMAT SourceFormat, 
        D3DFORMAT TargetFormat);
      
      STDMETHOD(GetDeviceCaps)(
        THIS_ UINT Adapter, 
        D3DDEVTYPE DeviceType, 
        D3DCAPS9* pCaps);
      
      STDMETHOD_(HMONITOR, GetAdapterMonitor)(
        THIS_ UINT Adapter);
      
      STDMETHOD(CreateDevice)(
        THIS_ UINT Adapter, 
        D3DDEVTYPE DeviceType, 
        HWND hFocusWindow, 
        DWORD BehaviorFlags, 
        D3DPRESENT_PARAMETERS* pPresentationParameters, 
        IDirect3DDevice9** ppReturnedDeviceInterface);
      
    private:
      IDirect3D9* m_pD3D;
    };
  }
}
  