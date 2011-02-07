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
#define D3D11_IGNORE_SDK_LAYERS
#include <d3d11.h>

namespace Hades
{
  namespace D3D11
  {
    class IDXGIFactoryHook : public IDXGIFactory
    {
    public:
      IDXGIFactoryHook(IDXGIFactory* pFactory)
        : m_pFactory(pFactory) 
      { }
      
      IDXGIFactory* GetFactory()
      {
        return m_pFactory;
      }
      
      virtual HRESULT STDMETHODCALLTYPE EnumAdapters( 
        UINT Adapter,
        IDXGIAdapter **ppAdapter);
        
      virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation( 
        HWND WindowHandle,
        UINT Flags);
        
      virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation( 
        HWND *pWindowHandle);
        
      virtual HRESULT STDMETHODCALLTYPE CreateSwapChain( 
        IUnknown *pDevice,
        DXGI_SWAP_CHAIN_DESC *pDesc,
        IDXGISwapChain **ppSwapChain);
        
      virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter( 
        HMODULE Module,
        IDXGIAdapter **ppAdapter);
        
    private:
      IDXGIFactory* m_pFactory;
    };
    
    class IDXGIFactory1Hook : public IDXGIFactory1
    {
    public:
      IDXGIFactory1Hook(IDXGIFactory1* pFactory)
        : m_pFactory(pFactory) 
      { }
      
      IDXGIFactory1* GetFactory()
      {
        return m_pFactory;
      }
      
      virtual HRESULT STDMETHODCALLTYPE EnumAdapters( 
        UINT Adapter,
        IDXGIAdapter **ppAdapter);
        
      virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation( 
        HWND WindowHandle,
        UINT Flags);
        
      virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation( 
        HWND *pWindowHandle);
        
      virtual HRESULT STDMETHODCALLTYPE CreateSwapChain( 
        IUnknown *pDevice,
        DXGI_SWAP_CHAIN_DESC *pDesc,
        IDXGISwapChain **ppSwapChain);
        
      virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter( 
        HMODULE Module,
        IDXGIAdapter **ppAdapter);
        
      virtual HRESULT STDMETHODCALLTYPE EnumAdapters1( 
        UINT Adapter,
        IDXGIAdapter1 **ppAdapter);
        
      virtual BOOL STDMETHODCALLTYPE IsCurrent();
        
    private:
      IDXGIFactory1* m_pFactory;
    };
  }
}
