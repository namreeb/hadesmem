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

// Boost C++ Libraries
#include <boost/thread.hpp>
#include <boost/format.hpp>

// Hades
#include "D3D9Hook.hpp"

namespace Hades
{
  namespace Kernel
  {
    // D3D9 hooks
    std::shared_ptr<Memory::PatchDetour> D3D9Hook::m_pResetHk;
    std::shared_ptr<Memory::PatchDetour> D3D9Hook::m_pReleaseHk;
    std::shared_ptr<Memory::PatchDetour> D3D9Hook::m_pEndSceneHk;
    std::shared_ptr<Memory::PatchDetour> D3D9Hook::m_pCreateDeviceHk;
    std::shared_ptr<Memory::PatchDetour> D3D9Hook::m_pDirect3DCreate9Hk;

    // Current device
    IDirect3DDevice9* D3D9Hook::m_pDevice;

    // State block
    IDirect3DStateBlock9* D3D9Hook::m_pStateBlock;

    // Callback managers
    D3D9Hook::Callbacks D3D9Hook::m_CallsOnFrame;
    D3D9Hook::Callbacks D3D9Hook::m_CallsOnRelease;
    D3D9Hook::Callbacks D3D9Hook::m_CallsOnLostDevice;
    D3D9Hook::Callbacks D3D9Hook::m_CallsOnInitialize;
    D3D9Hook::Callbacks D3D9Hook::m_CallsOnResetDevice;
      
    // D3D9 module handle
    Windows::EnsureFreeLibrary D3D9Hook::m_D3D9Mod;

    // Hook D3D9
    void D3D9Hook::Startup()
    {
      // Hook if required
      if (!m_pDirect3DCreate9Hk)
      {
        // Load D3D9
        // Todo: Defer hooking until game loads D3D9 (e.g. via LoadLibrary 
        // hook).
        m_D3D9Mod = LoadLibrary(_T("d3d9.dll"));
        if (!m_D3D9Mod)
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("D3D9Hook::Startup") << 
            ErrorString("Could not load D3D9.") << 
            ErrorCode(LastError));
        }

        // Get address of Direct3DCreate9
        FARPROC pDirect3DCreate9 = GetProcAddress(m_D3D9Mod, 
          "Direct3DCreate9");
        if (!pDirect3DCreate9)
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("D3D9Hook::Startup") << 
            ErrorString("Could not get address of Direct3DCreate9.") << 
            ErrorCode(LastError));
        }

        // Target and detour pointer
        PBYTE Target = reinterpret_cast<PBYTE>(pDirect3DCreate9);
        PBYTE Detour = reinterpret_cast<PBYTE>(&Direct3DCreate9_Hook);

        // Debug output
        std::wcout << "D3D9Hook::Startup: Hooking d3d9.dll!Direct3DCreate9." << 
          std::endl;
        std::wcout << boost::wformat(L"D3D9Hook::Startup: Target = %p, "
          L"Detour = %p.") %Target %Detour << std::endl;
          
        // Create memory manager
        Memory::MemoryMgr MyMemory(GetCurrentProcessId());

        // Hook Direct3DCreate9
        m_pDirect3DCreate9Hk.reset(new Memory::PatchDetour(MyMemory, Target, 
          Detour));
        m_pDirect3DCreate9Hk->Apply();
      }
    }
    
    // Unhook D3D9
    void D3D9Hook::Shutdown()
    {
      // Unhook all APIs
      if (m_pResetHk)
      {
        m_pResetHk->Remove();
      }
      if (m_pReleaseHk)
      {
        m_pReleaseHk->Remove();
      }
      if (m_pEndSceneHk)
      {
        m_pEndSceneHk->Remove();
      }
      if (m_pCreateDeviceHk)
      {
        m_pCreateDeviceHk->Remove();
      }
      if (m_pDirect3DCreate9Hk)
      {
        m_pDirect3DCreate9Hk->Remove();
      }
    }

    // Direct3DCreate9 hook implementation
    IDirect3D9* WINAPI D3D9Hook::Direct3DCreate9_Hook(UINT SDKVersion)
    {
      try
      {
        // Debug output
        std::wcout << "D3D9Hook::Direct3DCreate9_Hook: Called." << std::endl;

        // Get trampoline
        typedef IDirect3D9* (WINAPI* tDirect3DCreate9)(UINT SDKVersion);
        auto pDirect3DCreate9 = reinterpret_cast<tDirect3DCreate9>(
          m_pDirect3DCreate9Hk->GetTrampoline());

        // Call trampoline
        IDirect3D9* pD3D9 = pDirect3DCreate9(SDKVersion);

        // Debug output
        std::wcout << boost::wformat(L"D3D9Hook::Direct3DCreate9_Hook: "
          L"SDKVersion = %u. Return = %p.") %SDKVersion %pD3D9 << std::endl;

        // Check if D3D9 creation succeeded
        if (pD3D9)
        {
          // Ensure multi-threaded games don't cause hooking race conditions
          static boost::mutex HookMutex;
          boost::lock_guard<boost::mutex> HookLock(HookMutex);

          // Hook if required
          if (!m_pCreateDeviceHk)
          {
            // Get VMT
            PBYTE* pDeviceVMT = *reinterpret_cast<PBYTE**>(pD3D9);

            // Target and detour pointer
            PBYTE pCreateDevice = pDeviceVMT[16];
            PBYTE pCreateDeviceHk = reinterpret_cast<PBYTE>(&CreateDevice_Hook);

            // Debug output
            std::wcout << "D3D9Hook::Direct3DCreate9_Hook: Hooking "
              "d3d9.dll!IDirect3D9::CreateDevice." << std::endl;

            // Create memory manager
            Memory::MemoryMgr MyMemory(GetCurrentProcessId());

            // Hook CreateDevice
            m_pCreateDeviceHk.reset(new Memory::PatchDetour(MyMemory, 
              pCreateDevice, pCreateDeviceHk));
            m_pCreateDeviceHk->Apply();
          }
        }

        // Return result from trampoline
        return pD3D9;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Hook::Direct3DCreate9_Hook: "
          "Error! %s.") %e.what() << std::endl;
      }

      return nullptr;
    }

    // IDirect3D9::CreateDevice hook implementation
    HRESULT WINAPI D3D9Hook::CreateDevice_Hook(IDirect3D9* pThis, 
      UINT Adapter, 
      D3DDEVTYPE DeviceType, 
      HWND hFocusWindow, 
      DWORD BehaviorFlags, 
      D3DPRESENT_PARAMETERS* pPresentationParameters, 
      IDirect3DDevice9** ppReturnedDeviceInterface)
    {
      try
      {
        // Debug output
        std::wcout << "D3D9Hook::CreateDevice_Hook: Called." << std::endl;

        // Get trampoline
        typedef HRESULT (WINAPI* tCreateDevice)(IDirect3D9* pThis, 
          UINT Adapter, 
          D3DDEVTYPE DeviceType, 
          HWND hFocusWindow, 
          DWORD BehaviorFlags, 
          D3DPRESENT_PARAMETERS* pPresentationParameters, 
          IDirect3DDevice9** ppReturnedDeviceInterface);
        auto pCreateDevice = reinterpret_cast<tCreateDevice>(
          m_pCreateDeviceHk->GetTrampoline());

        // Call trampoline
        HRESULT Result = pCreateDevice(pThis, Adapter, DeviceType, hFocusWindow, 
          BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

        // Debug output
        std::wcout << boost::wformat(L"D3D9Hook::CreateDevice_Hook: "
          L"pThis = %p, Adapter = %u, DeviceType = %u, hFocusWindow = %p, "
          L"BehaviorFlags = %u, pPresentationParameters = %p, "
          L"ppReturnedDeviceInterface = %p. Return = %u.") %pThis %Adapter 
          %DeviceType %hFocusWindow %BehaviorFlags %pPresentationParameters 
          %ppReturnedDeviceInterface %Result << std::endl;

        // Check if device creation succeeded
        if (SUCCEEDED(Result))
        {
          // Ensure multi-threaded games don't cause hooking race conditions
          static boost::mutex HookMutex;
          boost::lock_guard<boost::mutex> HookLock(HookMutex);

          // Get device and VMT
          IDirect3DDevice9* pDevice = *ppReturnedDeviceInterface;
          PBYTE* pDeviceVMT = *reinterpret_cast<PBYTE**>(pDevice);

          // Hook if required
          if (!m_pEndSceneHk)
          {
            // Target and detour pointer
            PBYTE pEndScene = pDeviceVMT[42];
            PBYTE pEndSceneHk = reinterpret_cast<PBYTE>(&EndScene_Hook);

            // Debug output
            std::wcout << "D3D9Hook::CreateDevice_Hook: Hooking "
              "d3d9.dll!IDirect3DDevice9::EndScene." << std::endl;

            // Create memory manager
            Memory::MemoryMgr MyMemory(GetCurrentProcessId());

            // Hook EndScene
            m_pEndSceneHk.reset(new Memory::PatchDetour(MyMemory, pEndScene, 
              pEndSceneHk));
            m_pEndSceneHk->Apply();
          }

          // Hook if required
          if (!m_pResetHk)
          {
            // Target and detour pointer
            PBYTE pReset = pDeviceVMT[16];
            PBYTE pResetHk = reinterpret_cast<PBYTE>(&Reset_Hook);

            // Debug output
            std::wcout << "D3D9Hook::CreateDevice_Hook: Hooking "
              "d3d9.dll!IDirect3DDevice9::Reset." << std::endl;

            // Create memory manager
            Memory::MemoryMgr MyMemory(GetCurrentProcessId());

            // Hook Reset
            m_pResetHk.reset(new Memory::PatchDetour(MyMemory, pReset, 
              pResetHk));
            m_pResetHk->Apply();
          }

          // Hook if required
          if (!m_pReleaseHk)
          {
            // Target and detour pointer
            PBYTE pRelease = pDeviceVMT[2];
            PBYTE pReleaseHk = reinterpret_cast<PBYTE>(&Release_Hook);

            // Debug output
            std::wcout << "D3D9Hook::CreateDevice_Hook: Hooking "
              "d3d9.dll!IDirect3DDevice9::Release." << std::endl;

            // Create memory manager
            Memory::MemoryMgr MyMemory(GetCurrentProcessId());

            // Hook Release
            m_pReleaseHk.reset(new Memory::PatchDetour(MyMemory, pRelease, 
              pReleaseHk));
            m_pReleaseHk->Apply();
          }

          // Set device pointer
          m_pDevice = *ppReturnedDeviceInterface;

          // Initialize
          Initialize();
        }

        // Return result from trampoline
        return Result;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Hook::CreateDevice_Hook: Error! %s.") 
          %e.what() << std::endl;
      }

      return E_FAIL;
    }

    // IDrect3DDevice9::EndScene hook implementation
    HRESULT WINAPI D3D9Hook::EndScene_Hook(IDirect3DDevice9* pThis)
    {
      try
      {
        // Perform rendering
        Render();

        // Get trampoline
        typedef HRESULT (WINAPI* tEndScene)(IDirect3DDevice9* pThis);
        auto pEndScene = reinterpret_cast<tEndScene>(m_pEndSceneHk->
          GetTrampoline());

        // Call trampoline
        return pEndScene(pThis);
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Hook::EndScene_Hook: Error! %s.") 
          %e.what() << std::endl;
      }

      return E_FAIL;
    }

    // IDrect3DDevice9::Reset hook implementation
    HRESULT WINAPI D3D9Hook::Reset_Hook(IDirect3DDevice9* pThis, 
      D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
      try
      {
        // Perform pre-reset duties
        PreReset();

        // Get trampoline
        typedef HRESULT (WINAPI* tReset)(IDirect3DDevice9* pThis, 
          D3DPRESENT_PARAMETERS* pPresentationParameters);
        auto pReset = reinterpret_cast<tReset>(m_pResetHk->GetTrampoline());

        // Call trampoline
        HRESULT Result = pReset(pThis, pPresentationParameters);

        // Perform post-reset duties
        PostReset();

        // Return result from trampoline
        return Result;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Hook::Reset_Hook: Error! %s.") 
          %e.what() << std::endl;
      }

      return E_FAIL;
    }

    // IDrect3DDevice9::Release hook implementation
    HRESULT WINAPI D3D9Hook::Release_Hook(IDirect3DDevice9* pThis)
    {
      try
      {
        // Perform release duties
        Release();

        // Get trampoline
        typedef HRESULT (WINAPI* tRelease)(IDirect3DDevice9* pThis);
        auto pRelease = reinterpret_cast<tRelease>(m_pReleaseHk->
          GetTrampoline());

        // Return result from trampoline
        return pRelease(pThis);
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Hook::Release_Hook: Error! %s.") 
          %e.what() << std::endl;
      }

      return E_FAIL;
    }

    // Initialize resources such as textures (managed and unmanaged), vertex 
    // buffers, and other D3D resources
    void D3D9Hook::Initialize()
    {
      // Create state block
      m_pDevice->CreateStateBlock(D3DSBT_ALL, &m_pStateBlock);

      // Call registered callbacks
      m_CallsOnInitialize(m_pDevice);
    }

    // Release all unmanaged resources
    void D3D9Hook::PreReset()
    {
      // Release state block if required
      if (m_pStateBlock)
      {
        // Release state block
        m_pStateBlock->Release();
        // Prevent double-free
        m_pStateBlock = NULL;
      }

      // Call registered callbacks
      m_CallsOnLostDevice(m_pDevice);
    }

    // Re-initialize all unmanaged resources
    void D3D9Hook::PostReset()
    {
      // Create state block
      m_pDevice->CreateStateBlock(D3DSBT_ALL, &m_pStateBlock);

      // Call registered callbacks
      m_CallsOnResetDevice(m_pDevice);
    }

    // Do rendering
    void D3D9Hook::Render()
    {
      // Capture state block
      m_pStateBlock->Capture();

      // Call registered callbacks
      m_CallsOnFrame(m_pDevice);

      // Apply captured state block
      m_pStateBlock->Apply();
    }

    void D3D9Hook::Release()
    {
      // Call registered callbacks
      m_CallsOnRelease(m_pDevice);
    }

    // Register callback for OnFrame event
    boost::signals2::connection D3D9Hook::RegisterOnFrame(
      Callbacks::slot_type const& Subscriber)
    {
      // Register callback and return connection
      return m_CallsOnFrame.connect(Subscriber);
    }

    // Register callback for OnLostDevice event
    boost::signals2::connection D3D9Hook::RegisterOnLostDevice(
      Callbacks::slot_type const& Subscriber)
    {
      // Register callback and return connection
      return m_CallsOnLostDevice.connect(Subscriber);
    }

    // Register callback for OnResetDevice event
    boost::signals2::connection D3D9Hook::RegisterOnResetDevice(
      Callbacks::slot_type const& Subscriber)
    {
      // Register callback and return connection
      return m_CallsOnResetDevice.connect(Subscriber);
    }

    // Register callback for OnInitialize event
    boost::signals2::connection D3D9Hook::RegisterOnInitialize(
      Callbacks::slot_type const& Subscriber)
    {
      // Register callback and return connection
      return m_CallsOnInitialize.connect(Subscriber);
    }

    // Register callback for OnRelease event
    boost::signals2::connection D3D9Hook::RegisterOnRelease(
      Callbacks::slot_type const& Subscriber)
    {
      // Register callback and return connection
      return m_CallsOnRelease.connect(Subscriber);
    }
  }
}
