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

// C++ Standard Library
#include <memory>
#include <iostream>

// Windows API
#include <crtdbg.h>
#include <Windows.h>

// Hades
#include "Kernel.hpp"
#include "D3D9Hook.hpp"
#include "HadesMemory/Memory.hpp"
#include "HadesCommon/Logger.hpp"

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

    // Hades version number
    std::wstring const VerNum(L"TRUNK");

    // Version and copyright output
#if defined(_M_X64)
    std::wcout << "Hades-Kernel AMD64 [Version " << VerNum << "]" << std::endl;
#elif defined(_M_IX86)
    std::wcout << "Hades-Kernel IA32 [Version " << VerNum << "]" << std::endl;
#else
#error "[Hades] Unsupported architecture."
#endif
    std::wcout << "Copyright (C) 2010 RaptorFactor. All rights reserved." << 
      std::endl;
    std::wcout << "Website: http://www.raptorfactor.com/, "
      "Email: raptorfactor@raptorfactor.com." << std::endl;
    std::wcout << "Built on " << __DATE__ << " at " << __TIME__ << "." << 
      std::endl << std::endl;
    
    // Debug output
    std::wcout << boost::wformat(L"Hades-Kernel::Initialize: Module = %p.") 
      %Module << std::endl;

    // Initialize kernel
    static Hades::Kernel::Kernel MyKernel;
    
    // Initialize D3D9
    Hades::Kernel::D3D9Hook::Startup();
  }
  catch (std::exception const& e)
  {
    // Dump error information
    std::cout << boost::diagnostic_information(e) << std::endl;
    
    // Terminate self
    TerminateProcess(GetCurrentProcess(), 0);
  }
  
  return 0;
}

BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/, 
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
