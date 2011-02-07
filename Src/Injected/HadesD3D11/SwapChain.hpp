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
    class IDXGISwapChainHook : public IDXGISwapChain
    {
    public:
      IDXGISwapChainHook(IUnknown* pDevice, IDXGISwapChain* pSwapChain, 
        DXGI_SWAP_CHAIN_DESC const* pDesc)
        : m_pDevice(pDevice), 
        m_pSwapChain(pSwapChain), 
        m_Desc()
      {
        if (pDesc)
        {
          m_Desc = *pDesc;
        }
      }
      
      IUnknown* GetDevice()
      {
        return m_pDevice;
      }
      
      IDXGISwapChain* GetSwapChain()
      {
        return m_pSwapChain;
      }
      
      BOOL IsWindowMode()
      {
        return m_Desc.Windowed;
      }
      
      HWND GetOutputWindow()
      {
        return m_Desc.OutputWindow;
      }
      
      UINT GetBufferCount()
      {
        return m_Desc.BufferCount;
      }
      
      UINT GetHeight()
      {
        return m_Desc.BufferDesc.Height;
      }
      
      UINT GetWidth()
      {
        return m_Desc.BufferDesc.Width;
      }
      
      // 
      
      virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid, 
        void** ppvObject);
        
      virtual ULONG STDMETHODCALLTYPE AddRef();
        
      virtual ULONG STDMETHODCALLTYPE Release();
      
      // 
      
      virtual HRESULT STDMETHODCALLTYPE GetParent(
        REFIID riid,
        void **ppParent);
      
      virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
        REFGUID Name,
        UINT *pDataSize,
        void *pData);
      
      virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
        REFGUID Name,
        UINT DataSize,
        const void *pData);
      
      virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        REFGUID Name,
        const IUnknown *pUnknown);
        
      // 
      
      virtual HRESULT STDMETHODCALLTYPE GetDevice(
        REFIID riid,
        void **ppDevice);
      
      // 
      
      virtual HRESULT STDMETHODCALLTYPE Present( 
        UINT SyncInterval,
        UINT Flags);

      virtual HRESULT STDMETHODCALLTYPE GetBuffer( 
        UINT Buffer,
        REFIID riid,
        void **ppSurface);

      virtual HRESULT STDMETHODCALLTYPE SetFullscreenState( 
        BOOL Fullscreen,
        IDXGIOutput *pTarget);

      virtual HRESULT STDMETHODCALLTYPE GetFullscreenState( 
        BOOL *pFullscreen,
        IDXGIOutput **ppTarget);

      virtual HRESULT STDMETHODCALLTYPE GetDesc( 
        DXGI_SWAP_CHAIN_DESC *pDesc);

      virtual HRESULT STDMETHODCALLTYPE ResizeBuffers( 
        UINT BufferCount,
        UINT Width,
        UINT Height,
        DXGI_FORMAT NewFormat,
        UINT SwapChainFlags);

      virtual HRESULT STDMETHODCALLTYPE ResizeTarget( 
        const DXGI_MODE_DESC *pNewTargetParameters);

      virtual HRESULT STDMETHODCALLTYPE GetContainingOutput( 
        IDXGIOutput **ppOutput);

      virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics( 
        DXGI_FRAME_STATISTICS *pStats);

      virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount( 
        UINT *pLastPresentCount);
      
      // 
      
    private:
  		IUnknown* m_pDevice;
  		IDXGISwapChain* m_pSwapChain;
  		DXGI_SWAP_CHAIN_DESC m_Desc;
    };
  }
}
