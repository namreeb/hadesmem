/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
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

// Windows API
#include <Windows.h>

// Hades
#include "Hooker.hpp"
#include "HadesMemory/Memory.hpp"
#include "HadesKernel/Kernel.hpp"
#include "HadesCommon/Filesystem.hpp"

// Initialize D3D11
extern "C" __declspec(dllexport) DWORD __stdcall Initialize(HMODULE Module, 
  Hades::Kernel::Kernel& MyKernel)
{
  // Function is not thread-safe
  static boost::mutex MyMutex;
  boost::lock_guard<boost::mutex> MyLock(MyMutex);
    
  // 'Real' initialization code
  auto InitReal = [&] () 
  {
    // Break to debugger if present
    if (IsDebuggerPresent())
    {
      DebugBreak();
    }
    
    // Debug output
    std::wcout << boost::wformat(L"Hades-D3D11::Initialize: Module Base = %p, "
      L"Path to Self = %s, Path to Bin = %s.") %Module 
      %Hades::Windows::GetSelfPath() %Hades::Windows::GetModulePath(nullptr) 
      << std::endl;
        
    // Initialize and hook D3D11
    Hades::D3D11::D3D11Hooker::Initialize(MyKernel);
    Hades::D3D11::D3D11Hooker::Hook();
  };
    
  try
  {
    // Call real init routine safely
    static boost::once_flag f = BOOST_ONCE_INIT;
    boost::call_once(f, InitReal);
  }
  catch (std::exception const& e)
  {
    // Dump error information
    std::cout << boost::diagnostic_information(e) << std::endl;
      
    // Failure
    return 1;
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
