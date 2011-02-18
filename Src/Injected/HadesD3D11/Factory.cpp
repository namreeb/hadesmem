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
#include "Factory.hpp"
#include "SwapChain.hpp"
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace D3D11
  {
    // 
    
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::QueryInterface(
      REFIID riid, 
      void** ppvObject)
    {
      return m_pFactory->QueryInterface(riid, ppvObject);
    }
      
    ULONG STDMETHODCALLTYPE IDXGIFactoryHook::AddRef()
    {
      return m_pFactory->AddRef();
    }
      
    ULONG STDMETHODCALLTYPE IDXGIFactoryHook::Release()
    {
      return m_pFactory->Release();
    }
    
    // 
    
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::SetPrivateData( 
      REFGUID Name,
      UINT DataSize,
      const void *pData)
    {
      return m_pFactory->SetPrivateData(Name, DataSize, pData);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::SetPrivateDataInterface( 
      REFGUID Name,
      const IUnknown *pUnknown)
    {
      return m_pFactory->SetPrivateDataInterface(Name, pUnknown);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::GetPrivateData( 
      REFGUID Name,
      UINT *pDataSize,
      void *pData)
    {
      return m_pFactory->GetPrivateData(Name, pDataSize, pData);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::GetParent( 
      REFIID riid,
      void **ppParent)
    {
      return m_pFactory->GetParent(riid, ppParent);
    }
        
    // 
    
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::EnumAdapters( 
      UINT Adapter,
      IDXGIAdapter **ppAdapter)
    {
      return m_pFactory->EnumAdapters(Adapter, ppAdapter);
    }
      
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::MakeWindowAssociation( 
      HWND WindowHandle,
      UINT Flags)
    {
      return m_pFactory->MakeWindowAssociation(WindowHandle, Flags);
    }
      
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::GetWindowAssociation( 
      HWND *pWindowHandle)
    {
      return m_pFactory->GetWindowAssociation(pWindowHandle);
    }
      
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::CreateSwapChain( 
      IUnknown *pDevice,
      DXGI_SWAP_CHAIN_DESC *pDesc,
      IDXGISwapChain **ppSwapChain)
    {
      HRESULT Ret = m_pFactory->CreateSwapChain(pDevice, pDesc, ppSwapChain);
      if (SUCCEEDED(Ret))
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"IDXGIFactory1Hook::CreateSwapChain: "
          L"Call successful." << std::endl);
            
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"IDXGIFactory1Hook::CreateSwapChain: "
          L"Hooking IDXGISwapChain." << std::endl);
            
        *ppSwapChain = new IDXGISwapChainHook(pDevice, *ppSwapChain, pDesc);
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"IDXGIFactory1Hook::CreateSwapChain: "
          L"Call failed." << std::endl);
      }
      
      return Ret;
    }
      
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::CreateSoftwareAdapter( 
      HMODULE Module,
      IDXGIAdapter **ppAdapter)
    {
      return m_pFactory->CreateSoftwareAdapter(Module, ppAdapter);
    }
    
    // 
    
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::QueryInterface(
      REFIID riid, 
      void** ppvObject)
    {
      return m_pFactory->QueryInterface(riid, ppvObject);
    }
      
    ULONG STDMETHODCALLTYPE IDXGIFactory1Hook::AddRef()
    {
      return m_pFactory->AddRef();
    }
      
    ULONG STDMETHODCALLTYPE IDXGIFactory1Hook::Release()
    {
      return m_pFactory->Release();
    }
    
    // 
    
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::SetPrivateData( 
      REFGUID Name,
      UINT DataSize,
      const void *pData)
    {
      return m_pFactory->SetPrivateData(Name, DataSize, pData);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::SetPrivateDataInterface( 
      REFGUID Name,
      const IUnknown *pUnknown)
    {
      return m_pFactory->SetPrivateDataInterface(Name, pUnknown);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::GetPrivateData( 
      REFGUID Name,
      UINT *pDataSize,
      void *pData)
    {
      return m_pFactory->GetPrivateData(Name, pDataSize, pData);
    }
    
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::GetParent( 
      REFIID riid,
      void **ppParent)
    {
      return m_pFactory->GetParent(riid, ppParent);
    }
        
    // 
    
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::EnumAdapters( 
      UINT Adapter,
      IDXGIAdapter **ppAdapter)
    {
      return m_pFactory->EnumAdapters(Adapter, ppAdapter);
    }
      
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::MakeWindowAssociation( 
      HWND WindowHandle,
      UINT Flags)
    {
      return m_pFactory->MakeWindowAssociation(WindowHandle, Flags);
    }
      
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::GetWindowAssociation( 
      HWND *pWindowHandle)
    {
      return m_pFactory->GetWindowAssociation(pWindowHandle);
    }
      
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::CreateSwapChain( 
      IUnknown *pDevice,
      DXGI_SWAP_CHAIN_DESC *pDesc,
      IDXGISwapChain **ppSwapChain)
    {
      HRESULT Ret = m_pFactory->CreateSwapChain(pDevice, pDesc, ppSwapChain);
      if (SUCCEEDED(Ret))
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"IDXGIFactory1Hook::CreateSwapChain: "
          L"Call successful." << std::endl);
            
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"IDXGIFactory1Hook::CreateSwapChain: "
          L"Hooking IDXGISwapChain." << std::endl);
            
        *ppSwapChain = new IDXGISwapChainHook(pDevice, *ppSwapChain, pDesc);
      }
      else
      {
        HADES_LOG_THREAD_SAFE(std::wcout << 
          L"IDXGIFactory1Hook::CreateSwapChain: "
          L"Call failed." << std::endl);
      }
      
      return Ret;
    }
      
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::CreateSoftwareAdapter( 
      HMODULE Module,
      IDXGIAdapter **ppAdapter)
    {
      return m_pFactory->CreateSoftwareAdapter(Module, ppAdapter);
    }
        
    HRESULT STDMETHODCALLTYPE IDXGIFactory1Hook::EnumAdapters1( 
      UINT Adapter,
      IDXGIAdapter1 **ppAdapter)
    {
      return m_pFactory->EnumAdapters1(Adapter, ppAdapter);
    }
    
    BOOL STDMETHODCALLTYPE IDXGIFactory1Hook::IsCurrent()
    {
      return m_pFactory->IsCurrent();
    }
  }
}
