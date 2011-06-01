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

// Windows API
#include <crtdbg.h>
#include <Windows.h>

// C++ Standard Library
#include <string>
#include <iostream>
#include <exception>

// Boost
#pragma warning(push, 1)
#include <boost/format.hpp>
#include <boost/exception/all.hpp>
#pragma warning(pop)

// Hades
#include "Speeder.h"
#include "Crosshair.h"
#include "Hades-D3D9/GuiMgr.h"
#include "Hades-Kernel/Kernel.h"
#include "Hades-Common/Filesystem.h"
#include "Hades-Common/StringBuffer.h"

// Initialize Hades-Input
extern "C" __declspec(dllexport) DWORD __stdcall Initialize(
  Hades::Kernel::Kernel* pKernel)
{
  try
  {
    // Break to debugger if present
    if (IsDebuggerPresent())
    {
      DebugBreak();
    }

    // Hades version number
    std::wstring const VerNum(L"TRUNK");

    // Version and copyright output
#if defined(_M_X64)
    std::wcout << "Hades-HXGenHack::Initialize: Hades-HXGenHack AMD64 "
      "[Version " << VerNum << "]" << std::endl;
#elif defined(_M_IX86)
    std::wcout << "Hades-HXGenHack::Initialize: Hades-HXGenHack IA32 "
      "[Version " << VerNum << "]" << std::endl;
#else
#error Unsupported platform!
#endif
    std::wcout << "Hades-HXGenHack::Initialize: Copyright (C) 2010 Cypherjb. "
      "All rights reserved." << std::endl;
    std::wcout << "Hades-HXGenHack::Initialize: Website: "
      "http://www.cypherjb.com/, Email: cypher.jb@gmail.com." << std::endl;
    std::wcout << "Hades-HXGenHack::Initialize: Built on " << __DATE__ << 
      " at " << __TIME__ << "." << std::endl;

    // Debug output
    std::wcout << boost::wformat(L"Hades-HXGenHack::Initialize: Kernel = %p.") 
      %pKernel << std::endl;

    // Initialize speedhack
    Hades::HXGenHack::Speeder::Startup(pKernel);

    // Initialize crosshair
    Hades::HXGenHack::Crosshair::Startup(pKernel);

    // Get current working directory
    std::wstring CurDir;
    if (!GetCurrentDirectory(MAX_PATH, Hades::Util::MakeStringBuffer(CurDir, 
      MAX_PATH)))
    {
      DWORD LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("Hades-HXGenHack::Initialize") << 
        Hades::ErrorString("Could not get current directory.") << 
        Hades::ErrorCodeWin(LastError));
    }

    // Set new working directory to GUI library folder
    std::wstring GuiPath((Hades::Windows::GetSelfDirPath() / L"/../Gui/").
      file_string());
    if (!SetCurrentDirectory(GuiPath.c_str()))
    {
      DWORD LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("Hades-HXGenHack::Initialize") << 
        Hades::ErrorString("Could not set current directory.") << 
        Hades::ErrorCodeWin(LastError));
    }

    // Load GUI
    pKernel->GetGuiMgr()->GetGui()->LoadInterfaceFromFile("HXGenHack.xml");

    // Restore old working directory
    if (!SetCurrentDirectory(CurDir.c_str()))
    {
      DWORD LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("Hades-HXGenHack::Initialize") << 
        Hades::ErrorString("Could not restore current directory.") << 
        Hades::ErrorCodeWin(LastError));
    }
  }
  catch (boost::exception const& e)
  {
    // Dump error information
    MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
      "Hades-HXGenHack", MB_OK);
  }
  catch (std::exception const& e)
  {
    // Dump error information
    MessageBoxA(NULL, e.what(), "Hades-HXGenHack", MB_OK);
  }

  // Success
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
