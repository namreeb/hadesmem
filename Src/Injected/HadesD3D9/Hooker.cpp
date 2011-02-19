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

// C++ Standard Library
#include <iostream>

// Boost
#include <boost/format.hpp>
#include <boost/thread.hpp>

// Hades
#include "Hooker.hpp"
#include "HadesMemory/Memory.hpp"

namespace Hades
{
  namespace D3D9
  {
    // Todo: Ensure hook removal on module unload
    // Todo: Deferred hooking via a LoadLibrary hook
    
    // Kernel instance
    Kernel::Kernel* D3D9Hooker::m_pKernel = nullptr;
      
    // Device instance
    IDirect3DDevice9* D3D9Hooker::m_pDevice = nullptr;
    
    // Renderer instance
    std::shared_ptr<GUI::D3D9Renderer> D3D9Hooker::m_pRenderer;
    
    // d3d9.dll!Direct3DCreate9 hook
    std::shared_ptr<Hades::Memory::PatchDetour> D3D9Hooker::
      m_pDirect3DCreate9Hk;
    
    // d3d9.dll!IDirect3D9::CreateDevice hook  
    std::shared_ptr<Hades::Memory::PatchDetour> D3D9Hooker::m_pCreateDeviceHk;
      
    // d3d9.dll!IDrect3DDevice9::EndScene hook
    std::shared_ptr<Hades::Memory::PatchDetour> D3D9Hooker::m_pEndSceneHk;
    // d3d9.dll!IDrect3DDevice9::Reset hook
    std::shared_ptr<Hades::Memory::PatchDetour> D3D9Hooker::m_pResetHk;
      
    // Initialize D3D9 hooker
    void D3D9Hooker::Initialize(Kernel::Kernel& MyKernel)
    {
      m_pKernel = &MyKernel;
    }
      
    // Hook D3D9
    void D3D9Hooker::Hook()
    {
      // Check if already hooked
      if (m_pDirect3DCreate9Hk || m_pCreateDeviceHk || m_pEndSceneHk || 
        m_pResetHk)
      {
        if (m_pDirect3DCreate9Hk)
        {
          m_pDirect3DCreate9Hk->Apply();
        }
        
        if (m_pCreateDeviceHk)
        {
          m_pCreateDeviceHk->Apply();
        }
        
        if (m_pEndSceneHk)
        {
          m_pEndSceneHk->Apply();
        }
        
        if (m_pResetHk)
        {
          m_pResetHk->Apply();
        }
        
        return;
      }
      
      // Get d3d9.dll module handle
      HMODULE const D3D9Mod = LoadLibrary(L"d3d9.dll");
      if (!D3D9Mod)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("D3D9Hooker::Hook") << 
          ErrorString("Could not load d3d9.dll!") << 
          ErrorCode(LastError));
      }
      
      // Find d3d9.dll!!Direct3DCreate9
      FARPROC const pDirect3DCreate9 = GetProcAddress(D3D9Mod, 
        "Direct3DCreate9");
      if (!pDirect3DCreate9)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("D3D9Hooker::Hook") << 
          ErrorString("Could not find d3d9.dll!!Direct3DCreate9") << 
          ErrorCode(LastError));
      }
      
      // Debug output
      std::wcout << boost::wformat(L"D3D9Hooker::Hook: Hooking "
        L"d3d9.dll!Direct3DCreate9 (%p).") %pDirect3DCreate9 
        << std::endl;
          
      // Hook d3d9.dll!!Direct3DCreate9
      Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
      m_pDirect3DCreate9Hk.reset(new Memory::PatchDetour(MyMemory, 
        pDirect3DCreate9, &Direct3DCreate9_Hook));
      m_pDirect3DCreate9Hk->Apply();
    }
    
    // Unhook D3D9
    void D3D9Hooker::Unhook()
    {
      if (m_pDirect3DCreate9Hk)
      {
        m_pDirect3DCreate9Hk->Remove();
      }
      
      if (m_pCreateDeviceHk)
      {
        m_pCreateDeviceHk->Remove();
      }
      
      if (m_pEndSceneHk)
      {
        m_pEndSceneHk->Remove();
      }
      
      if (m_pResetHk)
      {
        m_pResetHk->Remove();
      }
    }
    
    // d3d9.dll!Direct3DCreate9 hook implementation
    IDirect3D9* WINAPI D3D9Hooker::Direct3DCreate9_Hook(UINT SDKVersion)
    {
      // Function is not thread-safe
      static boost::mutex MyMutex;
      boost::lock_guard<boost::mutex> MyLock(MyMutex);
        
      try
      {
        // Call trampoline
        typedef IDirect3D9* (WINAPI* tDirect3DCreate9)(UINT SDKVersion);
        auto const pDirect3DCreate9 = reinterpret_cast<tDirect3DCreate9>(
          m_pDirect3DCreate9Hk->GetTrampoline());      
        IDirect3D9* pD3D9 = pDirect3DCreate9(SDKVersion);
          
        // Debug output
        std::wcout << boost::wformat(L"D3D9Hooker::Direct3DCreate9_Hook: "
          L"SDKVersion = %u. Return = %p.") %SDKVersion %pD3D9 << std::endl;
            
        // Hook if call successful
        if (pD3D9 && !m_pCreateDeviceHk)
        {
          // Get pointer to d3d9.dll!IDirect3D9::CreateDevice
          PBYTE* pInterfaceVMT = *reinterpret_cast<PBYTE**>(pD3D9);
          PVOID pCreateDevice = pInterfaceVMT[16];
          
          // Debug output
          std::wcout << boost::wformat(L"D3D9Hooker::Direct3DCreate9_Hook: "
            L"Hooking d3d9.dll!IDirect3D9::CreateDevice (%p).") %pCreateDevice 
            << std::endl;

          // Hook CreateDevice
          Memory::MemoryMgr MyMemory(GetCurrentProcessId());
          m_pCreateDeviceHk.reset(new Hades::Memory::PatchDetour(MyMemory, 
            pCreateDevice, &CreateDevice_Hook));
          m_pCreateDeviceHk->Apply();
        }
        
        // Return result from trampoline
        return pD3D9;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Hooker::Direct3DCreate9_Hook: "
          "Error! %s.") %e.what() << std::endl;
            
        // Failure
        return nullptr;
      }
    }
    
    // d3d9.dll!IDirect3D9::CreateDevice hook implementation
    HRESULT WINAPI D3D9Hooker::CreateDevice_Hook(
      IDirect3D9* pThis, 
      UINT Adapter, 
      D3DDEVTYPE DeviceType, 
      HWND hFocusWindow, 
      DWORD BehaviorFlags, 
      D3DPRESENT_PARAMETERS* pPresentationParameters, 
      IDirect3DDevice9** ppReturnedDeviceInterface)
    {
      // Function is not thread-safe
      static boost::mutex MyMutex;
      boost::lock_guard<boost::mutex> MyLock(MyMutex);
        
      try
      {
        // Call trampoline
        typedef HRESULT (WINAPI* tCreateDevice)(IDirect3D9* pThis, 
          UINT Adapter, 
          D3DDEVTYPE DeviceType, 
          HWND hFocusWindow, 
          DWORD BehaviorFlags, 
          D3DPRESENT_PARAMETERS* pPresentationParameters, 
          IDirect3DDevice9** ppReturnedDeviceInterface);
        auto pCreateDevice = reinterpret_cast<tCreateDevice>(m_pCreateDeviceHk->
          GetTrampoline());
        HRESULT Result = pCreateDevice(pThis, Adapter, DeviceType, hFocusWindow, 
          BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

        // Debug output
        std::wcout << boost::wformat(L"D3D9Hooker::CreateDevice_Hook: "
          L"pThis = %p, Adapter = %u, DeviceType = %u, hFocusWindow = %p, "
          L"BehaviorFlags = %u, pPresentationParameters = %p, "
          L"ppReturnedDeviceInterface = %p. Return = %u.") 
          %pThis %Adapter %DeviceType %hFocusWindow %BehaviorFlags 
          %pPresentationParameters %ppReturnedDeviceInterface %Result 
          << std::endl;
            
        // Hook if call successful
        if (SUCCEEDED(Result) && !m_pEndSceneHk && !m_pResetHk)
        {
          // Get device and VMT
          IDirect3DDevice9* pDevice = *ppReturnedDeviceInterface;
          PBYTE* pDeviceVMT = *reinterpret_cast<PBYTE**>(pDevice);

          // Get pointer to d3d9.dll!IDirect3DDevice9::EndScene
          PVOID pEndScene = pDeviceVMT[42];
          
          // Debug output
          std::wcout << boost::wformat(L"D3D9Hooker::Direct3DCreate9_Hook: "
            L"Hooking d3d9.dll!IDirect3DDevice9::EndScene (%p).") %pEndScene 
            << std::endl;

          // Create memory manager
          Memory::MemoryMgr MyMemory(GetCurrentProcessId());

          // Hook EndScene
          m_pEndSceneHk.reset(new Hades::Memory::PatchDetour(MyMemory, 
            pEndScene, &EndScene_Hook));
          m_pEndSceneHk->Apply();

          // Get pointer to d3d9.dll!IDirect3DDevice9::Reset
          PVOID pReset = pDeviceVMT[16];
          
          // Debug output
          std::wcout << boost::wformat(L"D3D9Hooker::Direct3DCreate9_Hook: "
            L"Hooking d3d9.dll!IDirect3DDevice9::Reset (%p).") %pReset 
            << std::endl;

          // Hook Reset
          m_pResetHk.reset(new Hades::Memory::PatchDetour(MyMemory, 
            pReset, &Reset_Hook));
          m_pResetHk->Apply();
        }
        
        // Perform initialization if call successfull
        if (SUCCEEDED(Result))
        {
          // Debug output
          std::wcout << boost::wformat(L"D3D9Hooker::CreateDevice_Hook: "
            L"Initializing resources. Old device = %p, New device = %p.") 
            %m_pDevice %*ppReturnedDeviceInterface << std::endl;
              
          // Cache device pointer
          m_pDevice = *ppReturnedDeviceInterface;
          
          // Create renderer
          m_pRenderer.reset(new GUI::D3D9Renderer(m_pDevice));
        }
        
        return Result;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Hooker::CreateDevice_Hook: "
          "Error! %s.") %e.what() << std::endl;
            
        // Failure
        return E_FAIL;
      }
    }
    
    // d3d9.dll!IDirect3DDevice9::EndScene hook implementation
    HRESULT WINAPI D3D9Hooker::EndScene_Hook(IDirect3DDevice9* pThis)
    {
      // Function is not thread-safe
      static boost::mutex MyMutex;
      boost::lock_guard<boost::mutex> MyLock(MyMutex);
        
      try
      {
        // Sanity check
        if (pThis != m_pDevice)
        {
          std::wcout << "D3D9Hooker::EndScene_Hook: Warning! Multiple devices "
            "are currently unsupported." << std::endl;
        }
        
        // Get trampoline
        typedef HRESULT (WINAPI* tEndScene)(IDirect3DDevice9* pThis);
        auto pEndScene = reinterpret_cast<tEndScene>(m_pEndSceneHk->
          GetTrampoline());

        // Notify kernel of OnFrame event
        m_pKernel->OnFrame(*m_pRenderer);

        // Call trampoline
        return pEndScene(pThis);
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Hooker::EndScene_Hook: "
          "Error! %s.") %e.what() << std::endl;
            
        // Failure
        return E_FAIL;
      }
    }

    // d3d9.dll!IDirect3DDevice9::Reset hook implementation
    HRESULT WINAPI D3D9Hooker::Reset_Hook(IDirect3DDevice9* pThis, 
      D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
      try
      {
        // Sanity check
        if (pThis != m_pDevice)
        {
          std::wcout << "D3D9Hooker::EndScene_Hook: Warning! Multiple devices "
            "are currently unsupported." << std::endl;
        }
        
        // Get trampoline
        typedef HRESULT (WINAPI* tReset)(IDirect3DDevice9* pThis, 
          D3DPRESENT_PARAMETERS* pPresentationParameters);
        auto pReset = reinterpret_cast<tReset>(m_pResetHk->GetTrampoline());

        // Perform pre-reset duties
        m_pRenderer->PreReset();

        // Call trampoline
        HRESULT Result = pReset(pThis, pPresentationParameters);

        // Perform post-reset duties
        m_pRenderer->PostReset();

        // Return result from trampoline
        return Result;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Hooker::Reset_Hook: "
          "Error! %s.") %e.what() << std::endl;
            
        // Failure
        return E_FAIL;
      }
    }
  }
}
