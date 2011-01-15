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
#include "HadesMemory/Memory.hpp"
#include "HadesCommon/Logger.hpp"

// Todo: Make hook objects a member of a class with a destructor that runs on 
// module unload to ensure that we call PatchDetour::Remove

std::shared_ptr<Hades::Memory::PatchDetour> GetCursorPosHk;

BOOL WINAPI GetCursorPos_Hook(
  LPPOINT lpPoint
)
{
  std::wcout << "GetCursorPos called." << std::endl;
  
  typedef BOOL (WINAPI* tGetCursorPos)(LPPOINT lpPoint);
  tGetCursorPos pGetCursorPos = reinterpret_cast<tGetCursorPos>(
    GetCursorPosHk->GetTrampoline());
  
  return pGetCursorPos(lpPoint);
}

extern "C" __declspec(dllexport) DWORD __stdcall Initialize(HMODULE /*Module*/)
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
    
    // Create memory manager
    Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
    // Test hooking
    HMODULE User32Mod = LoadLibrary(_T("user32.dll"));
    if (!User32Mod)
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("_Initialize@4") << 
        Hades::ErrorString("Could not load user32.dll.") << 
        Hades::ErrorCode(LastError));
    }
    FARPROC pGetCursorPos = GetProcAddress(User32Mod, "GetCursorPos");
    if (!pGetCursorPos)
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("_Initialize@4") << 
        Hades::ErrorString("Could not find user32.dll!GetCursorPos.") << 
        Hades::ErrorCode(LastError));
    }
    GetCursorPosHk.reset(new Hades::Memory::PatchDetour(
      MyMemory, 
      reinterpret_cast<PVOID>(pGetCursorPos), 
      reinterpret_cast<PVOID>(&GetCursorPos_Hook)));
    GetCursorPosHk->Apply();
  }
  catch (std::exception const& e)
  {
    // Dump error information
    std::cout << boost::diagnostic_information(e);
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
