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

// Boost
#include <boost/format.hpp>

// Hades
#include "Device.hpp"
#include "Interface.hpp"
#include "HadesCommon/Error.hpp"
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace D3D9
  {
    IDirect3D9Hook::IDirect3D9Hook(IDirect3D9* pD3D9) 
      : m_pD3D(pD3D9)
    {
      if (!pD3D9)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("IDirect3DDevice9Hook::IDirect3DDevice9Hook") << 
          ErrorString("Invalid interface pointer.") << 
          ErrorCode(LastError));
      }
    }
    
    HRESULT APIENTRY IDirect3D9Hook::QueryInterface(
      REFIID riid, 
      void** ppvObj)
    {
      return m_pD3D->QueryInterface(
        riid, 
        ppvObj);
    }
    
    ULONG APIENTRY IDirect3D9Hook::AddRef()
    {
      return m_pD3D->AddRef();
    }
    
    HRESULT APIENTRY IDirect3D9Hook::CheckDepthStencilMatch(
      UINT Adapter, 
      D3DDEVTYPE DeviceType, 
      D3DFORMAT AdapterFormat, 
      D3DFORMAT RenderTargetFormat, 
      D3DFORMAT DepthStencilFormat)
    {
      return m_pD3D->CheckDepthStencilMatch(
        Adapter, 
        DeviceType, 
        AdapterFormat, 
        RenderTargetFormat, 
        DepthStencilFormat);
    }
    
    HRESULT APIENTRY IDirect3D9Hook::CheckDeviceFormat(
      UINT Adapter, 
      D3DDEVTYPE DeviceType, 
      D3DFORMAT AdapterFormat, 
      DWORD Usage, 
      D3DRESOURCETYPE RType, 
      D3DFORMAT CheckFormat)
    {
      return m_pD3D->CheckDeviceFormat(
        Adapter, 
        DeviceType, 
        AdapterFormat, 
        Usage, 
        RType, 
        CheckFormat);
    }
    
    HRESULT APIENTRY IDirect3D9Hook::CheckDeviceFormatConversion(
      UINT Adapter, 
      D3DDEVTYPE DeviceType, 
      D3DFORMAT SourceFormat, 
      D3DFORMAT TargetFormat)
    {
      return m_pD3D->CheckDeviceFormatConversion(
        Adapter, 
        DeviceType, 
        SourceFormat, 
        TargetFormat);
    }
    
    HRESULT APIENTRY IDirect3D9Hook::CheckDeviceMultiSampleType(
      UINT Adapter, 
      D3DDEVTYPE DeviceType, 
      D3DFORMAT SurfaceFormat, 
      BOOL Windowed, 
      D3DMULTISAMPLE_TYPE MultiSampleType, 
      DWORD* pQualityLevels)
    {
      return m_pD3D->CheckDeviceMultiSampleType(
        Adapter, 
        DeviceType, 
        SurfaceFormat, 
        Windowed, 
        MultiSampleType, 
        pQualityLevels);
    }
    
    HRESULT APIENTRY IDirect3D9Hook::CheckDeviceType(
      UINT Adapter, 
      D3DDEVTYPE CheckType, 
      D3DFORMAT DisplayFormat, 
      D3DFORMAT BackBufferFormat, 
      BOOL Windowed)
    {
      return m_pD3D->CheckDeviceType(
        Adapter, 
        CheckType, 
        DisplayFormat, 
        BackBufferFormat, 
        Windowed);
    }
    
    HRESULT APIENTRY IDirect3D9Hook::CreateDevice(
      UINT Adapter, 
      D3DDEVTYPE DeviceType, 
      HWND hFocusWindow, 
      DWORD BehaviorFlags, 
      D3DPRESENT_PARAMETERS* pPresentationParameters, 
      IDirect3DDevice9** ppReturnedDeviceInterface)
    {
      HRESULT Result = m_pD3D->CreateDevice(
        Adapter, 
        DeviceType, 
        hFocusWindow, 
        BehaviorFlags, 
        pPresentationParameters, 
        ppReturnedDeviceInterface);
        
      HADES_LOG_THREAD_SAFE(std::wcout << boost::wformat(L"IDirect3D9Hook::"
        L"CreateDevice: Result = %u.") %Result << std::endl);
          
      if (SUCCEEDED(Result))
      {
        HADES_LOG_THREAD_SAFE(std::wcout << boost::wformat(L"IDirect3D9Hook::"
          L"CreateDevice: Hooking device. Device = %p.") 
          %*ppReturnedDeviceInterface << std::endl);
        *ppReturnedDeviceInterface = new IDirect3DDevice9Hook(
          this, 
          *ppReturnedDeviceInterface, 
          pPresentationParameters);
      }
    
      return Result;
    }
    
    HRESULT APIENTRY IDirect3D9Hook::EnumAdapterModes(
      UINT Adapter, 
      D3DFORMAT Format, 
      UINT Mode, 
      D3DDISPLAYMODE* pMode)
    {
      return m_pD3D->EnumAdapterModes(
        Adapter, 
        Format, 
        Mode, 
        pMode);
    }
    
    UINT APIENTRY IDirect3D9Hook::GetAdapterCount()
    {
      return m_pD3D->GetAdapterCount();
    }
    
    HRESULT APIENTRY IDirect3D9Hook::GetAdapterDisplayMode(
      UINT Adapter, 
      D3DDISPLAYMODE *pMode)
    {
      return m_pD3D->GetAdapterDisplayMode(
        Adapter, 
        pMode);
    }
    
    HRESULT APIENTRY IDirect3D9Hook::GetAdapterIdentifier(
      UINT Adapter, 
      DWORD Flags, 
      D3DADAPTER_IDENTIFIER9 *pIdentifier)
    {
      return m_pD3D->GetAdapterIdentifier(
        Adapter, 
        Flags, 
        pIdentifier);
    }
    
    UINT APIENTRY IDirect3D9Hook::GetAdapterModeCount(
      UINT Adapter, 
      D3DFORMAT Format)
    {
      return m_pD3D->GetAdapterModeCount(Adapter, Format);
    }
    
    HMONITOR APIENTRY IDirect3D9Hook::GetAdapterMonitor(
      UINT Adapter)
    {
      return m_pD3D->GetAdapterMonitor(
        Adapter);
    }
    
    HRESULT APIENTRY IDirect3D9Hook::GetDeviceCaps(
      UINT Adapter, 
      D3DDEVTYPE DeviceType, 
      D3DCAPS9 *pCaps)
    {
      return m_pD3D->GetDeviceCaps(
        Adapter, 
        DeviceType, 
        pCaps);
    }
    
    HRESULT APIENTRY IDirect3D9Hook::RegisterSoftwareDevice(
      void *pInitializeFunction)
    {
      return m_pD3D->RegisterSoftwareDevice(
        pInitializeFunction);
    }
    
    ULONG APIENTRY IDirect3D9Hook::Release()
    {
      return m_pD3D->Release();
    }
  }
}
