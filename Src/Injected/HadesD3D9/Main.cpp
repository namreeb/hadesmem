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

// Windows API
#include <Windows.h>

// Boost
#include <boost/format.hpp>

// Hades
#include "Exports.hpp"
#include "HadesMemory/Memory.hpp"
#include "HadesCommon/Logger.hpp"
#include "HadesCommon/Filesystem.hpp"

// Initialize D3D9
extern "C" __declspec(dllexport) DWORD __stdcall Initialize(HMODULE Module)
{
  try
  {
    // Break to debugger if present
    if (IsDebuggerPresent())
    {
      DebugBreak();
    }
    
    // Initialize logger
    Hades::Util::InitLogger(L"Log", L"Debug");
      
    // Debug output
    HADES_LOG_THREAD_SAFE(std::wcout << boost::wformat(
      L"Hades-D3D9::Initialize: Module Base = %p, Path to Self = %s, "
      L"Path to Bin = %s.") %Module %Hades::Windows::GetSelfPath() 
      %Hades::Windows::GetModulePath(nullptr) << std::endl);
        
    // Hook D3D9
    Hades::D3D9::D3D9Manager::Hook();
  }
  catch (std::exception const& e)
  {
    // Dump error information
    std::cout << boost::diagnostic_information(e) << std::endl;
  }

  // Success
  return 0;
}

// Entry point
BOOL WINAPI DllMain(
  HINSTANCE /*hinstDLL*/, 
  DWORD /*fdwReason*/, 
  LPVOID /*lpvReserved*/)
{
  // Attempt to detect memory leaks in debug mode
#ifdef _DEBUG
  int CurrentFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  int NewFlags = (_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | 
    _CRTDBG_CHECK_ALWAYS_DF);
  _CrtSetDbgFlag(CurrentFlags | NewFlags);
#endif
    
  return TRUE;
}
