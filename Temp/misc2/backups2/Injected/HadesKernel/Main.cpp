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

// Windows API
#include <Windows.h>

// Boost
#include <boost/thread.hpp>
#include <boost/format.hpp>

// Hades
#include "Kernel.hpp"
#include "Loader.hpp"
#include "HadesCommon/Logger.hpp"
#include "HadesCommon/Filesystem.hpp"

// Initialize kernel
// Todo: Ensure safe for reentry
extern "C" __declspec(dllexport) DWORD __stdcall Initialize(HMODULE Module)
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
    
    // Initialize logger
    Hades::Util::InitLogger(L"Log", L"Debug");
      
    // Debug output
    std::wcout << boost::wformat(L"Hades-Kernel::Initialize: "
      L"Module Base = %p, Path to Self = %s, Path to Bin = %s.") %Module 
      %Hades::Windows::GetSelfPath() %Hades::Windows::GetModulePath(nullptr) 
      << std::endl;
        
    // Initialize Kernel
    static Hades::Kernel::Kernel MyKernel;
      
    // Initialize Loader
    Hades::Kernel::Loader::Initialize(MyKernel);
    Hades::Kernel::Loader::Hook();
      
    // Load D3D extensions
    MyKernel.LoadExtension("HadesD3D9.dll");
    MyKernel.LoadExtension("HadesD3D11.dll");
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
