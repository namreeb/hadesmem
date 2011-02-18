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

// Hades
#include "SwapChain.hpp"
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace D3D11
  {
    // 
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::QueryInterface(
      REFIID riid, 
      void** ppvObject)
    {
      return m_pSwapChain->QueryInterface(riid, ppvObject);
    }
      
    ULONG STDMETHODCALLTYPE IDXGISwapChainHook::AddRef()
    {
      return m_pSwapChain->AddRef();
    }
      
    ULONG STDMETHODCALLTYPE IDXGISwapChainHook::Release()
    {
      return m_pSwapChain->Release();
    }
    
    // 
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::GetParent(
      REFIID riid,
      void **ppParent)
    {
      return m_pSwapChain->GetParent(riid, ppParent);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::GetPrivateData(
      REFGUID Name,
      UINT *pDataSize,
      void *pData)
    {
      return m_pSwapChain->GetPrivateData(Name, pDataSize, pData);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::SetPrivateData(
      REFGUID Name,
      UINT DataSize,
      const void *pData)
    {
      return m_pSwapChain->SetPrivateData(Name, DataSize, pData);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::SetPrivateDataInterface(
      REFGUID Name,
      const IUnknown *pUnknown)
    {
      return m_pSwapChain->SetPrivateDataInterface(Name, pUnknown);
    }
        
    // 
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::GetDevice(
      REFIID riid,
      void **ppDevice)
    {
      return m_pSwapChain->GetDevice(riid, ppDevice);
    }
       
    // 
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::Present( 
      UINT SyncInterval,
      UINT Flags)
    {
      HADES_LOG_THREAD_SAFE(std::wcout << L"IDXGISwapChainHook::Present: "
        L"Called." << std::endl);
      return m_pSwapChain->Present(SyncInterval, Flags);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::GetBuffer( 
      UINT Buffer,
      REFIID riid,
      void **ppSurface)
    {
      return m_pSwapChain->GetBuffer(Buffer, riid, ppSurface);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::SetFullscreenState( 
      BOOL Fullscreen,
      IDXGIOutput *pTarget)
    {
      return m_pSwapChain->SetFullscreenState(Fullscreen, pTarget);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::GetFullscreenState( 
      BOOL *pFullscreen,
      IDXGIOutput **ppTarget)
    {
      return m_pSwapChain->GetFullscreenState(pFullscreen, ppTarget);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::GetDesc( 
      DXGI_SWAP_CHAIN_DESC *pDesc)
    {
      return m_pSwapChain->GetDesc(pDesc);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::ResizeBuffers( 
      UINT BufferCount,
      UINT Width,
      UINT Height,
      DXGI_FORMAT NewFormat,
      UINT SwapChainFlags)
    {
      return m_pSwapChain->ResizeBuffers(BufferCount, Width, Height, NewFormat, 
        SwapChainFlags);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::ResizeTarget( 
      const DXGI_MODE_DESC *pNewTargetParameters)
    {
      return m_pSwapChain->ResizeTarget(pNewTargetParameters);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::GetContainingOutput( 
      IDXGIOutput **ppOutput)
    {
      return m_pSwapChain->GetContainingOutput(ppOutput);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::GetFrameStatistics( 
      DXGI_FRAME_STATISTICS *pStats)
    {
      return m_pSwapChain->GetFrameStatistics(pStats);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGISwapChainHook::GetLastPresentCount( 
      UINT *pLastPresentCount)
    {
      return m_pSwapChain->GetLastPresentCount(pLastPresentCount);
    }
    
    // 
  }
}
