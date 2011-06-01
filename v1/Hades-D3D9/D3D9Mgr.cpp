/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

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
#pragma warning(push, 1)
#include <boost/thread.hpp>
#pragma warning(pop)

// Hades
#include "D3D9Mgr.h"
#include "Hades-Memory/Patcher.h"
#include "Hades-Input/InputMgr.h"

namespace Hades
{
  namespace D3D9
  {
    // D3D9 hooks
    std::shared_ptr<Memory::PatchDetour> D3D9Mgr::m_pResetHk;
    std::shared_ptr<Memory::PatchDetour> D3D9Mgr::m_pReleaseHk;
    std::shared_ptr<Memory::PatchDetour> D3D9Mgr::m_pEndSceneHk;
    std::shared_ptr<Memory::PatchDetour> D3D9Mgr::m_pCreateDeviceHk;
    std::shared_ptr<Memory::PatchDetour> D3D9Mgr::m_pDirect3DCreate9Hk;

    // Current device
    IDirect3DDevice9* D3D9Mgr::m_pDevice;

    // Current device window
    HWND D3D9Mgr::m_WindowCur;

    // State block
    IDirect3DStateBlock9* D3D9Mgr::m_pStateBlock;

    // D3D9 helper
    D3D9HelperPtr D3D9Mgr::m_pD3D9Helper;

    // Callback managers
    D3D9Mgr::Callbacks D3D9Mgr::m_CallsOnFrame;
    D3D9Mgr::Callbacks D3D9Mgr::m_CallsOnRelease;
    D3D9Mgr::Callbacks D3D9Mgr::m_CallsOnLostDevice;
    D3D9Mgr::Callbacks D3D9Mgr::m_CallsOnInitialize;
    D3D9Mgr::Callbacks D3D9Mgr::m_CallsOnResetDevice;

    // Hades manager
    Kernel::Kernel* D3D9Mgr::m_pKernel = nullptr;

    // Hook D3D9
    void D3D9Mgr::Startup(Kernel::Kernel* pKernel)
    {
      // Set HadesMgr pointer
      m_pKernel = pKernel;

      // Hook if required
      if (!m_pDirect3DCreate9Hk && m_pKernel->IsHookEnabled(
        L"d3d9.dll!Direct3DCreate9"))
      {
        // Load D3D9
        // Todo: Defer hooking until game loads D3D9 (e.g. via LoadLibrary 
        // hook).
        HMODULE D3D9Mod = LoadLibrary(L"d3d9.dll");
        if (!D3D9Mod)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(D3D9MgrError() << 
            ErrorFunction("D3D9Mgr::Startup") << 
            ErrorString("Could not load D3D9.") << 
            ErrorCodeWin(LastError));
        }

        // Get address of Direct3DCreate9
        FARPROC pDirect3DCreate9 = GetProcAddress(D3D9Mod, "Direct3DCreate9");
        if (!pDirect3DCreate9)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(D3D9MgrError() << 
            ErrorFunction("D3D9Mgr::Startup") << 
            ErrorString("Could not get address of Direct3DCreate9.") << 
            ErrorCodeWin(LastError));
        }

        // Target and detour pointer
        PBYTE Target = reinterpret_cast<PBYTE>(pDirect3DCreate9);
        PBYTE Detour = reinterpret_cast<PBYTE>(&Direct3DCreate9_Hook);

        // Debug output
        std::wcout << "D3D9Mgr::Startup: Hooking d3d9.dll!Direct3DCreate9." << 
          std::endl;
        std::wcout << boost::wformat(L"D3D9Mgr::Startup: Target = %p, "
          L"Detour = %p.") %Target %Detour << std::endl;

        // Hook Direct3DCreate9
        m_pDirect3DCreate9Hk.reset(new Hades::Memory::PatchDetour(*pKernel->
          GetMemoryMgr(), Target, Detour));
        m_pDirect3DCreate9Hk->Apply();
      }
    }

    // Direct3DCreate9 hook implementation
    IDirect3D9* WINAPI D3D9Mgr::Direct3DCreate9_Hook(UINT SDKVersion)
    {
      try
      {
        // Get trampoline
        typedef IDirect3D9* (WINAPI* tDirect3DCreate9)(UINT SDKVersion);
        auto pDirect3DCreate9 = reinterpret_cast<tDirect3DCreate9>(
          m_pDirect3DCreate9Hk->GetTrampoline());

        // Debug output
        std::wcout << "D3D9Mgr::Direct3DCreate9_Hook: Called." << std::endl;

        // Call trampoline
        IDirect3D9* pD3D9 = pDirect3DCreate9(SDKVersion);

        // Debug output
        std::wcout << boost::wformat(L"D3D9Mgr::Direct3DCreate9_Hook: "
          L"SDKVersion = %u. Return = %p.") %SDKVersion %pD3D9 << std::endl;

        // Check if D3D9 creation succeeded
        if (pD3D9)
        {
          // Ensure multi-threaded games don't cause hooking race conditions
          static boost::mutex HookMutex;
          boost::lock_guard<boost::mutex> HookLock(HookMutex);

          // Hook if required
          if (!m_pCreateDeviceHk && m_pKernel->IsHookEnabled(
            L"d3d9.dll!IDirect3D9::CreateDevice"))
          {
            // Get VMT
            PBYTE* pDeviceVMT = *reinterpret_cast<PBYTE**>(pD3D9);

            // Target and detour pointer
            PBYTE pCreateDevice = pDeviceVMT[16];
            PBYTE pCreateDeviceHk = reinterpret_cast<PBYTE>(&CreateDevice_Hook);

            // Debug output
            std::wcout << "D3D9Mgr::Direct3DCreate9_Hook: Hooking "
              "d3d9.dll!IDirect3D9::CreateDevice." << std::endl;

            // Hook CreateDevice
            m_pCreateDeviceHk.reset(new Hades::Memory::PatchDetour(*m_pKernel->
              GetMemoryMgr(), pCreateDevice, pCreateDeviceHk));
            m_pCreateDeviceHk->Apply();
          }
        }

        // Return result from trampoline
        return pD3D9;
      }
      catch (boost::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Mgr::Direct3DCreate9_Hook: Error! %s.") 
          %boost::diagnostic_information(e) << std::endl;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Mgr::Direct3DCreate9_Hook: Error! %s.") 
          %e.what() << std::endl;
      }

      return nullptr;
    }

    // IDirect3D9::CreateDevice hook implementation
    HRESULT WINAPI D3D9Mgr::CreateDevice_Hook(IDirect3D9* pThis, 
      UINT Adapter, 
      D3DDEVTYPE DeviceType, 
      HWND hFocusWindow, 
      DWORD BehaviorFlags, 
      D3DPRESENT_PARAMETERS* pPresentationParameters, 
      IDirect3DDevice9** ppReturnedDeviceInterface)
    {
      try
      {
        // Get trampoline
        typedef HRESULT (WINAPI* tCreateDevice)(IDirect3D9* pThis, 
          UINT Adapter, 
          D3DDEVTYPE DeviceType, 
          HWND hFocusWindow, 
          DWORD BehaviorFlags, 
          D3DPRESENT_PARAMETERS* pPresentationParameters, 
          IDirect3DDevice9** ppReturnedDeviceInterface);
        auto pCreateDevice = reinterpret_cast<tCreateDevice>(m_pCreateDeviceHk->
          GetTrampoline());

        // Debug output
        std::wcout << "D3D9Mgr::CreateDevice_Hook: Called." << std::endl;

        // Hook window
        m_pKernel->GetInputMgr()->HookWindow(hFocusWindow);

        // Set current window
        m_WindowCur = hFocusWindow;

        // Call trampoline
        HRESULT Result = pCreateDevice(pThis, Adapter, DeviceType, hFocusWindow, 
          BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

        // Debug output
        std::wcout << boost::wformat(L"D3D9Mgr::CreateDevice_Hook: pThis = %p, "
          L"Adapter = %u, DeviceType = %u, hFocusWindow = %p, BehaviorFlags = "
          L"%u, pPresentationParameters = %p, ppReturnedDeviceInterface = %p. "
          L"Return = %u.") %pThis %Adapter %DeviceType %hFocusWindow 
          %BehaviorFlags %pPresentationParameters %ppReturnedDeviceInterface 
          %Result << std::endl;

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
          if (!m_pEndSceneHk && m_pKernel->IsHookEnabled(
            L"d3d9.dll!IDirect3DDevice9::EndScene"))
          {
            // Target and detour pointer
            PBYTE pEndScene = pDeviceVMT[42];
            PBYTE pEndSceneHk = reinterpret_cast<PBYTE>(&EndScene_Hook);

            // Debug output
            std::wcout << "D3D9Mgr::CreateDevice_Hook: Hooking "
              "d3d9.dll!IDirect3DDevice9::EndScene." << std::endl;

            // Hook EndScene
            m_pEndSceneHk.reset(new Hades::Memory::PatchDetour(*m_pKernel->
              GetMemoryMgr(), pEndScene, pEndSceneHk));
            m_pEndSceneHk->Apply();
          }

          // Hook if required
          if (!m_pResetHk && m_pKernel->IsHookEnabled(
            L"d3d9.dll!IDirect3DDevice9::Reset"))
          {
            // Target and detour pointer
            PBYTE pReset = pDeviceVMT[16];
            PBYTE pResetHk = reinterpret_cast<PBYTE>(&Reset_Hook);

            // Debug output
            std::wcout << "D3D9Mgr::CreateDevice_Hook: Hooking "
              "d3d9.dll!IDirect3DDevice9::Reset." << std::endl;

            // Hook Reset
            m_pResetHk.reset(new Hades::Memory::PatchDetour(*m_pKernel->
              GetMemoryMgr(), pReset, pResetHk));
            m_pResetHk->Apply();
          }

          // Hook if required
          if (!m_pReleaseHk && m_pKernel->IsHookEnabled(
            L"d3d9.dll!IDirect3DDevice9::Release"))
          {
            // Target and detour pointer
            PBYTE pRelease = pDeviceVMT[2];
            PBYTE pReleaseHk = reinterpret_cast<PBYTE>(&Release_Hook);

            // Debug output
            std::wcout << "D3D9Mgr::CreateDevice_Hook: Hooking "
              "d3d9.dll!IDirect3DDevice9::Release." << std::endl;

            // Hook Release
            m_pReleaseHk.reset(new Hades::Memory::PatchDetour(*m_pKernel->
              GetMemoryMgr(), pRelease, pReleaseHk));
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
      catch (boost::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Mgr::CreateDevice_Hook: Error! %s.") 
          %boost::diagnostic_information(e) << std::endl;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Mgr::CreateDevice_Hook: Error! %s.") 
          %e.what() << std::endl;
      }

      return E_FAIL;
    }

    // IDrect3DDevice9::EndScene hook implementation
    HRESULT WINAPI D3D9Mgr::EndScene_Hook(IDirect3DDevice9* pThis)
    {
      try
      {
        // Get trampoline
        typedef HRESULT (WINAPI* tEndScene)(IDirect3DDevice9* pThis);
        auto pEndScene = reinterpret_cast<tEndScene>(m_pEndSceneHk->
          GetTrampoline());

        // Perform rendering
        Render();

        // Call trampoline
        return pEndScene(pThis);
      }
      catch (boost::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Mgr::EndScene_Hook: Error! %s.") 
          %boost::diagnostic_information(e) << std::endl;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Mgr::EndScene_Hook: Error! %s.") 
          %e.what() << std::endl;
      }

      return E_FAIL;
    }

    // IDrect3DDevice9::Reset hook implementation
    HRESULT WINAPI D3D9Mgr::Reset_Hook(IDirect3DDevice9* pThis, 
      D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
      try
      {
        // Get trampoline
        typedef HRESULT (WINAPI* tReset)(IDirect3DDevice9* pThis, 
          D3DPRESENT_PARAMETERS* pPresentationParameters);
        auto pReset = reinterpret_cast<tReset>(m_pResetHk->GetTrampoline());

        // Perform pre-reset duties
        PreReset();

        // Call trampoline
        HRESULT Result = pReset(pThis, pPresentationParameters);

        // Perform post-reset duties
        PostReset();

        // Return result from trampoline
        return Result;
      }
      catch (boost::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Mgr::Reset_Hook: Error! %s.") 
          %boost::diagnostic_information(e) << std::endl;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Mgr::Reset_Hook: Error! %s.") 
          %e.what() << std::endl;
      }

      return E_FAIL;
    }

    // IDrect3DDevice9::Release hook implementation
    HRESULT WINAPI D3D9Mgr::Release_Hook(IDirect3DDevice9* pThis)
    {
      try
      {
        // Get trampoline
        typedef HRESULT (WINAPI* tRelease)(IDirect3DDevice9* pThis);
        auto pRelease = reinterpret_cast<tRelease>(m_pReleaseHk->
          GetTrampoline());

        // Perform release duties
        Release();

        // Return result from trampoline
        return pRelease(pThis);
      }
      catch (boost::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Mgr::Release_Hook: Error! %s.") 
          %boost::diagnostic_information(e) << std::endl;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("D3D9Mgr::Release_Hook: Error! %s.") 
          %e.what() << std::endl;
      }

      return E_FAIL;
    }

    // Initialize resources such as textures (managed and unmanaged), vertex 
    // buffers, and other D3D resources
    void D3D9Mgr::Initialize()
    {
      // Create new GUI manager
      m_pD3D9Helper.reset(new D3D9Helper());
      m_pD3D9Helper->OnInitialize(m_pDevice, m_pD3D9Helper);

      // Create state block
      m_pDevice->CreateStateBlock(D3DSBT_ALL, &m_pStateBlock);

      // Call registered callbacks
      m_CallsOnInitialize(m_pDevice, m_pD3D9Helper);
    }

    // Release all unmanaged resources
    void D3D9Mgr::PreReset()
    {
      // Release state block if required
      if (m_pStateBlock)
      {
        // Release state block
        m_pStateBlock->Release();
        // Prevent double-free
        m_pStateBlock = NULL;
      }

      // D3D9 Helper
      m_pD3D9Helper->OnLostDevice(m_pDevice, m_pD3D9Helper);

      // Call registered callbacks
      m_CallsOnLostDevice(m_pDevice, m_pD3D9Helper);
    }

    // Re-initialize all unmanaged resources
    void D3D9Mgr::PostReset()
    {
      // Create state block
      m_pDevice->CreateStateBlock(D3DSBT_ALL, &m_pStateBlock);

      // D3D9 Helper
      m_pD3D9Helper->OnResetDevice(m_pDevice, m_pD3D9Helper);

      // Call registered callbacks
      m_CallsOnResetDevice(m_pDevice, m_pD3D9Helper);
    }

    // Do rendering
    void D3D9Mgr::Render()
    {
      // Capture state block
      m_pStateBlock->Capture();

      // Call registered callbacks
      m_CallsOnFrame(m_pDevice, m_pD3D9Helper);

      // Apply captured state block
      m_pStateBlock->Apply();
    }

    void D3D9Mgr::Release()
    {
      // Call registered callbacks
      m_CallsOnRelease(m_pDevice, m_pD3D9Helper);
    }

    // Register callback for OnFrame event
    boost::signals2::connection D3D9Mgr::RegisterOnFrame(
      Callbacks::slot_type const& Subscriber )
    {
      // Register callback and return connection
      return m_CallsOnFrame.connect(Subscriber);
    }

    // Register callback for OnLostDevice event
    boost::signals2::connection D3D9Mgr::RegisterOnLostDevice(
      Callbacks::slot_type const& Subscriber )
    {
      // Register callback and return connection
      return m_CallsOnLostDevice.connect(Subscriber);
    }

    // Register callback for OnResetDevice event
    boost::signals2::connection D3D9Mgr::RegisterOnResetDevice(
      Callbacks::slot_type const& Subscriber )
    {
      // Register callback and return connection
      return m_CallsOnResetDevice.connect(Subscriber);
    }

    // Register callback for OnInitialize event
    boost::signals2::connection D3D9Mgr::RegisterOnInitialize(
      Callbacks::slot_type const& Subscriber )
    {
      // Register callback and return connection
      return m_CallsOnInitialize.connect(Subscriber);
    }

    // Register callback for OnRelease event
    boost::signals2::connection D3D9Mgr::RegisterOnRelease(
      Callbacks::slot_type const& Subscriber)
    {
      // Register callback and return connection
      return m_CallsOnRelease.connect(Subscriber);
    }

    // Get current device window
    HWND D3D9Mgr::GetDeviceWindow()
    {
      return m_WindowCur;
    }
  }
}
