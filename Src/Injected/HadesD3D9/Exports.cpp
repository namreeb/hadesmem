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

// Hades
#include "Exports.hpp"
#include "Interface.hpp"
#include "HadesMemory/Memory.hpp"
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace D3D9
  {
    Kernel::Kernel* D3D9Manager::m_pKernel = nullptr;
      
    std::shared_ptr<Hades::Memory::PatchDetour> D3D9Manager::
      m_pDirect3DCreate9;
      
    void D3D9Manager::Initialize(Kernel::Kernel* pKernel)
    {
      m_pKernel = pKernel;
    }
      
    void D3D9Manager::Hook()
    {
      // Todo: Deferred hooking via a LoadLibrary hook
      HMODULE const D3D9Mod = LoadLibrary(L"d3d9.dll");
      if (!D3D9Mod)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("D3D9Manager::Hook") << 
          ErrorString("Could not load D3D9.dll") << 
          ErrorCode(LastError));
      }
      
      FARPROC const pDirect3DCreate9 = GetProcAddress(D3D9Mod, 
        "Direct3DCreate9");
      if (!pDirect3DCreate9)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("D3D9Manager::Hook") << 
          ErrorString("Could not find D3D9!Direct3DCreate9") << 
          ErrorCode(LastError));
      }
      
      HADES_LOG_THREAD_SAFE(std::wcout << boost::wformat(L"D3D9Manager::"
        L"Hook: pDirect3DCreate9 = %p.") %pDirect3DCreate9 << std::endl);
          
      Memory::MemoryMgr MyMemory(GetCurrentProcessId());
      m_pDirect3DCreate9.reset(new Memory::PatchDetour(MyMemory, pDirect3DCreate9, 
        &Direct3DCreate9_Hook));
      m_pDirect3DCreate9->Apply();
    }
    
    void D3D9Manager::Unhook()
    {
      if (m_pDirect3DCreate9)
      {
        m_pDirect3DCreate9->Remove();
      }
    }
    
    IDirect3D9* WINAPI D3D9Manager::Direct3DCreate9_Hook(UINT SDKVersion)
    {
      HADES_LOG_THREAD_SAFE(std::wcout << boost::wformat(L"D3D9Manager::"
        L"Direct3DCreate9_Hook: SDKVersion = %u.") %SDKVersion << std::endl);
          
      typedef IDirect3D9* (WINAPI* tDirect3DCreate9)(UINT SDKVersion);
      auto pDirect3DCreate9 = reinterpret_cast<tDirect3DCreate9>(
        m_pDirect3DCreate9->GetTrampoline());      
      IDirect3D9* pD3D9 = pDirect3DCreate9(SDKVersion);
        
      HADES_LOG_THREAD_SAFE(std::wcout << boost::wformat(L"D3D9Manager::"
        L"Direct3DCreate9_Hook: Return = %p.") %pD3D9 << std::endl);
          
      return new IDirect3D9Hook(m_pKernel, pD3D9);
    }
  }
}
