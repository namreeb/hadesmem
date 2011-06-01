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
#include <string>
#include <iostream>
#include <exception>
#include <algorithm>

// Boost
#include <boost/timer.hpp>
#include <boost/format.hpp>
#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>

// Windows
#include <Windows.h>
#include <Shellapi.h>

// Hades
#include <HadesCommon/I18n.hpp>
#include <HadesCommon/Config.hpp>
#include <HadesMemory/Memory.hpp>

#ifdef HADES_GCC
int wmain(int argc, wchar_t* argv[]);

int CALLBACK wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, 
  LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
  LPWSTR CmdLineW = GetCommandLine();
  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(CmdLineW, &argc);
  try
  {
    int result = wmain(argc, argv);
    LocalFree(argv);
    return result;
  }
  catch (...)
  {
    LocalFree(argv);
  }
  
  return 0;
}
#endif

// Program entry point
int wmain(int argc, wchar_t* argv[])
{
  // Program timer
  boost::timer const ProgTimer;

  try
  {
    // Attempt to detect memory leaks in debug mode
#ifdef _DEBUG
    int const CurrentFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    int NewFlags = (_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | 
      _CRTDBG_CHECK_ALWAYS_DF);
    _CrtSetDbgFlag(CurrentFlags | NewFlags);

    // Get default heap
    HANDLE const ProcHeap = GetProcessHeap();
    if (!ProcHeap)
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("Could not get process heap.") << 
        Hades::ErrorCode(LastError));
    }

    // Detect heap corruption
    if (!HeapSetInformation(ProcHeap, HeapEnableTerminationOnCorruption, 
      NULL, 0))
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("Could not set heap information.") << 
        Hades::ErrorCode(LastError));
    }
#endif

    // Hades version number
    std::wstring const VerNum(L"TRUNK");
    
    // Version and copyright output
#if defined(_M_X64)
    std::wcout << "Hades CreateAndInjectDll AMD64 [Version " << VerNum << 
      "]\n";
#elif defined(_M_IX86)
    std::wcout << "Hades CreateAndInjectDll IA32 [Version " << VerNum << 
      "]\n";
#else
#error "[HadesMem] Unsupported architecture."
#endif
    std::wcout << "Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).\n"
      << "<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>\n"
      << "Compiler: \"" << BOOST_COMPILER << "\", Standard Library: \"" 
      << BOOST_STDLIB << "\", Platform: \"" << BOOST_PLATFORM << "\", Boost: " 
      << BOOST_VERSION << ".\n";
    std::wcout << "Built on " << __DATE__ << " at " << __TIME__ << ".\n" 
      << std::endl;
    
    // Get target process from args
    if (argc < 2)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a valid process path."));
    }
    boost::filesystem::path ProcPath;
    try
    {
      ProcPath = argv[1];
    }
    catch (std::exception const& /*e*/)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a valid process path."));
    }
    
    // Get target process args from args
    if (argc < 3)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply valid process args."));
    }
    std::wstring ProcArgs;
    try
    {
      ProcArgs = argv[2];
    }
    catch (std::exception const& /*e*/)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply valid process args."));
    }
    
    // Get target module from args
    if (argc < 4)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a valid module path."));
    }
    boost::filesystem::path ModulePath;
    try
    {
      ModulePath = argv[3];
    }
    catch (std::exception const& /*e*/)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a valid module path."));
    }
    
    // Get target export from args (optional)
    std::string Export;
    if (argc > 4)
    {
      Export = boost::lexical_cast<std::string>(static_cast<std::wstring>(
        argv[4]));
    }
    
    // Create process and inject DLL
    std::unique_ptr<Hades::Memory::CreateAndInjectData> pInjectionData;
    if (ModulePath.is_absolute())
    {
      if (boost::filesystem::exists(ModulePath))
      {
        std::wcout << "Absolute module path detected, and file located. "
          "Attempting injection without path resolution.\n";
      }
      else
      {
        std::wcout << "Absolute module path detected, but file could not be. "
          "located. Attempting injection without path resolution anyway.\n";
      }
      
      pInjectionData.reset(new Hades::Memory::CreateAndInjectData(
        Hades::Memory::CreateAndInject(ProcPath, "", ProcArgs, ModulePath, 
        Export, false)));
    }
    else
    {
      if (boost::filesystem::exists(boost::filesystem::absolute(ModulePath)))
      {
        std::wcout << "Relative module path detected, and file located. "
          "Attempting injection with path resolution.\n";
      }
      else
      {
        std::wcout << "Relative module path detected, but file could not be "
          "located. Attempting injection with path resolution anyway.\n";
      }
      
      pInjectionData.reset(new Hades::Memory::CreateAndInjectData(
        Hades::Memory::CreateAndInject(ProcPath, "", ProcArgs, ModulePath, 
        Export, true)));
    }
    
    HMODULE ModuleBase = pInjectionData->GetModule();
    DWORD_PTR ExportRet = pInjectionData->GetExportRet();
    DWORD ExportLastError = pInjectionData->GetExportLastError();
    if (!Export.empty())
    {
      std::wcout << boost::wformat(L"Module successfully injected. Base = %p, "
        L"Return = %d (%x), LastError = %d (%x).\n") %ModuleBase %ExportRet 
        %ExportRet %ExportLastError %ExportLastError;
    }
    else
    {
      std::wcout << boost::wformat(L"Module successfully injected. Base = "
        L"%p.\n") %ModuleBase;
    }
  }
  catch (std::exception const& e)
  {
    std::cerr << boost::diagnostic_information(e) << std::endl;
  }
  
  // Print elapsed time
  std::wcout << "\nElapsed Time: " << ProgTimer.elapsed() << "\n";
  
  return 0;
}
