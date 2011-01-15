 /*
This file is part of HadesMem.
Copyright © 2010 RaptorFactor (aka Cypherjb, Cypher, Chazwazza). 
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

// C++ Standard Library
#include <iostream>

// Boost
#include <boost/format.hpp>

// Hades
#include "D3D9Hook.hpp"
#include "HadesMemory/Memory.hpp"

namespace Hades
{
  namespace Kernel
  {
    // Current device
    IDirect3DDevice9* D3D9Hook::m_pDevice = nullptr;

    // State block
    IDirect3DStateBlock9*	D3D9Hook::m_pStateBlock = nullptr;

    // Hooks
    std::shared_ptr<Memory::PatchDetour> D3D9Hook::m_pResetHk;
    std::shared_ptr<Memory::PatchDetour> D3D9Hook::m_pEndSceneHk;
      
    // Start D3D9 engine
    void D3D9Hook::Startup()
    {
      // Debug output
      std::wcout << "D3D9Hook::Startup: Loading D3D9." << std::endl;
      
      // Ensure D3D9 is loaded
      HMODULE ModD3D9 = LoadLibrary(_T("d3d9.dll"));
      if (!ModD3D9)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("D3D9Hook::Startup") << 
          ErrorString("Could not load D3D9 module.") << 
          ErrorCode(LastError));
      }
      
      // Debug output
      std::wcout << "D3D9Hook::Startup: Preparing to hook D3D9." << std::endl;
      
      // Initialize symbol API
      Memory::MemoryMgr MyMemory(GetCurrentProcessId());
      Memory::Symbols MySymbols(MyMemory);
      MySymbols.LoadForModule(_T("d3d9.dll"));
      PVOID pEndScene = MySymbols.GetAddress(_T("d3d9!CD3DBase::EndScene"));
      PVOID pReset = MySymbols.GetAddress(_T("d3d9!CBaseDevice::Reset"));
      
      // Debug output
      std::wcout << "D3D9Hook::Startup: Hooking D3D9." << std::endl;
      std::wcout << boost::wformat(L"D3D9Hook::Startup: "
        L"d3d9!CD3DBase::EndScene = %p.") %pEndScene << std::endl;
      std::wcout << boost::wformat(L"D3D9Hook::Startup: "
        L"d3d9!CBaseDevice::Reset = %p.") %pReset << std::endl;
      
      // Hook IDrect3DDevice9::EndScene
      m_pEndSceneHk.reset(new Memory::PatchDetour(MyMemory, pEndScene, 
        reinterpret_cast<PVOID>(&EndScene_Hook)));
      m_pEndSceneHk->Apply();
      
      // Hook IDrect3DDevice9::Reset
      m_pResetHk.reset(new Memory::PatchDetour(MyMemory, pReset, 
        reinterpret_cast<PVOID>(&Reset_Hook)));
      m_pEndSceneHk->Apply();
        
      // Debug output
      std::wcout << "D3D9Hook::Startup: D3D9 successfully hooked." 
        << std::endl;
    }
    
    // Stop D3D9 engine
    void D3D9Hook::Shutdown()
    {
      if (m_pEndSceneHk)
      {
        m_pEndSceneHk->Remove();
      }
      
      if (m_pResetHk)
      {
        m_pResetHk->Remove();
      }
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
      
    
    // Stop D3D9 engine
    void D3D9Hook::Initialize()
    {
      // Create new GUI manager
      //m_pD3D9Helper.reset(new D3D9Helper());
      //m_pD3D9Helper->OnInitialize(m_pDevice, m_pD3D9Helper);

      // Create state block
      m_pDevice->CreateStateBlock(D3DSBT_ALL, &m_pStateBlock);

      // Call registered callbacks
      //m_CallsOnInitialize(m_pDevice, m_pD3D9Helper);
    }
    
    // Stop D3D9 engine
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

      // D3D9 Helper
      //m_pD3D9Helper->OnLostDevice(m_pDevice, m_pD3D9Helper);

      // Call registered callbacks
      //m_CallsOnLostDevice(m_pDevice, m_pD3D9Helper);
    }
    
    // Stop D3D9 engine
    void D3D9Hook::PostReset()
    {
      // Create state block
      m_pDevice->CreateStateBlock(D3DSBT_ALL, &m_pStateBlock);

      // D3D9 Helper
      //m_pD3D9Helper->OnResetDevice(m_pDevice, m_pD3D9Helper);

      // Call registered callbacks
      //m_CallsOnResetDevice(m_pDevice, m_pD3D9Helper);
    }
    
    // Stop D3D9 engine
    void D3D9Hook::Render()
    {
      // Capture state block
      m_pStateBlock->Capture();

      // Call registered callbacks
      //m_CallsOnFrame(m_pDevice, m_pD3D9Helper);

      // Apply captured state block
      m_pStateBlock->Apply();
      
      // Debug output
      std::wcout << "D3D9Hook::Render: Called." << std::endl;
    }
  }
}
