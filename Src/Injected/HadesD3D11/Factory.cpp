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
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace D3D11
  {
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
      HADES_LOG_THREAD_SAFE(std::wcout << L"IDXGIFactoryHook::"
        L"CreateSwapChain: Test." << std::endl);
      return m_pFactory->CreateSwapChain(pDevice, pDesc, ppSwapChain);
    }
      
    HRESULT STDMETHODCALLTYPE IDXGIFactoryHook::CreateSoftwareAdapter( 
      HMODULE Module,
      IDXGIAdapter **ppAdapter)
    {
      return m_pFactory->CreateSoftwareAdapter(Module, ppAdapter);
    }
    
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
      HADES_LOG_THREAD_SAFE(std::wcout << L"IDXGIFactory1Hook::"
        L"CreateSwapChain: Test." << std::endl);
      return m_pFactory->CreateSwapChain(pDevice, pDesc, ppSwapChain);
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
