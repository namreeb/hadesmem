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
#include <string>
#include <memory>
#include <iostream>

// Windows
#include <Windows.h>
#include <atlbase.h>
#include <atlwin.h>

// WTL
#include <atlapp.h>
#include <atlddx.h>
#include <atluser.h>
#include <atlmisc.h>
#include <atlframe.h>
#include <atlcrack.h>
#include <atlctrls.h>

// Hades
#include "Window.h"
#include "GameLoop.h"
#include "Hades-Common/Error.h"
#include "Hades-Common/Logger.h"
#include "Hades-Common/EnsureCleanup.h"

int CALLBACK wWinMain(
  __in  HINSTANCE hInstance,
  __in  HINSTANCE /*hPrevInstance*/,
  __in  LPWSTR /*lpCmdLine*/,
  __in  int nCmdShow)
{
  try
  {
    // Initialize COM
    HRESULT CoInitResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(CoInitResult))
    {
      DWORD LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wWinMain") << 
        Hades::ErrorString("Could not initialize COM.") << 
        Hades::ErrorCodeWin(LastError));
    }
    Hades::Windows::EnsureCoUninitialize MyEnsureCoUninitalize;

    // Initialize logger
    Hades::Util::InitLogger(L"Hades-GUISandbox-Log", 
      L"Hades-GUISandbox-Debug");

    // Hades version number
    std::wstring const VerNum(L"TRUNK");

    // Version and copyright output
#if defined(_M_X64)
    std::wcout << "Hades-GUISandbox AMD64 [Version " << VerNum << "]" 
      << std::endl;
#elif defined(_M_IX86)
    std::wcout << "Hades-GUISandbox IA32 [Version " << VerNum << "]" 
      << std::endl;
#else
#error Unsupported platform!
#endif
    std::wcout << "Copyright (C) 2010 Cypherjb. All rights reserved." << 
      std::endl;
    std::wcout << "Website: http://www.cypherjb.com/, Email: "
      "cypher.jb@gmail.com." << std::endl;
    std::wcout << "Built on " << __DATE__ << " at " << __TIME__ << "." << 
      std::endl;

    // Initialize common controls
    if (!AtlInitCommonControls(ICC_BAR_CLASSES))
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wWinMain") << 
        Hades::ErrorString("Could not initialize common controls."));
    }

    // Initialize app module
    CAppModule MyAppModule;
    HRESULT InitAppModResult = MyAppModule.Init(NULL, hInstance);
    if (FAILED(InitAppModResult))
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wWinMain") << 
        Hades::ErrorString("Could not initialize app module."));
    }

    // Add message loop
    Hades::GUISandbox::GameLoop MyMessageLoop;
    if (!MyAppModule.AddMessageLoop(&MyMessageLoop))
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wWinMain") << 
        Hades::ErrorString("Could not add message loop."));
    }

    // Message loop result
    int Result = 0;

    {
      // Create loader window manager
      Hades::GUISandbox::SandboxWindow MainWindow(&MyAppModule);

      // Create window
      if (!MainWindow.CreateEx())
      {
        BOOST_THROW_EXCEPTION(Hades::HadesError() << 
          Hades::ErrorFunction("wWinMain") << 
          Hades::ErrorString("Could not create loader window."));
      }

      // Initialize D3D
      MainWindow.InitD3D();

      // Load GUI
      MainWindow.LoadGUI();

      // Show window
      MainWindow.ShowWindow(nCmdShow);

      // Run message loop
      Result = MyMessageLoop.Run();
    }

    // Terminate app module
    MyAppModule.Term();

    // Return exit code from message loop
    return Result;
  }
  catch (boost::exception const& e)
  {
    // Dump error information
    MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
      "Hades-GUISandbox", MB_OK);
  }
  catch (std::exception const& e)
  {
    // Dump error information
    MessageBoxA(NULL, e.what(), "Hades-GUISandbox", MB_OK);
  }

  return 0;
}
