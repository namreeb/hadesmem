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
#include <HadesMemory/Memory.hpp>
#include <HadesMemory/Detail/I18n.hpp>
#include <HadesMemory/Experimental/ManualMap.hpp>

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
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(HadesMem::HadesMemError() << 
        HadesMem::ErrorFunction("wmain") << 
        HadesMem::ErrorString("Could not get process heap.") << 
        HadesMem::ErrorCodeWinLast(LastError));
    }

    // Detect heap corruption
    if (!HeapSetInformation(ProcHeap, HeapEnableTerminationOnCorruption, 
      NULL, 0))
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(HadesMem::HadesMemError() << 
        HadesMem::ErrorFunction("wmain") << 
        HadesMem::ErrorString("Could not set heap information.") << 
        HadesMem::ErrorCodeWinLast(LastError));
    }
#endif

    // Version and copyright output
#if defined(_M_X64)
    std::wcout << "Hades ManualMapper AMD64 [Version " << 
      HADES_VERSION_FULL_STRING << "]\n";
#elif defined(_M_IX86)
    std::wcout << "Hades ManualMapper IA32 [Version " << 
      HADES_VERSION_FULL_STRING << "]\n";
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
    
    // Get target process ID from args
    if (argc < 2)
    {
      BOOST_THROW_EXCEPTION(HadesMem::HadesMemError() << 
        HadesMem::ErrorFunction("wmain") << 
        HadesMem::ErrorString("You must supply a valid process id."));
    }
    DWORD ProcId = 0;
    try
    {
      ProcId = boost::lexical_cast<DWORD>(argv[1]);
    }
    catch (std::exception const& /*e*/)
    {
      BOOST_THROW_EXCEPTION(HadesMem::HadesMemError() << 
        HadesMem::ErrorFunction("wmain") << 
        HadesMem::ErrorString("You must supply a valid process id."));
    }
    
    // Get target module from args
    if (argc < 3)
    {
      BOOST_THROW_EXCEPTION(HadesMem::HadesMemError() << 
        HadesMem::ErrorFunction("wmain") << 
        HadesMem::ErrorString("You must supply a valid module path."));
    }
    boost::filesystem::path ModulePath;
    try
    {
      ModulePath = argv[2];
    }
    catch (std::exception const& /*e*/)
    {
      BOOST_THROW_EXCEPTION(HadesMem::HadesMemError() << 
        HadesMem::ErrorFunction("wmain") << 
        HadesMem::ErrorString("You must supply a valid module path."));
    }
    
    // Get target export from args (optional)
    std::string Export;
    if (argc > 3)
    {
      Export = boost::lexical_cast<std::string>(static_cast<std::wstring>(
        argv[3]));
    }
    
    // Open memory manager
    HadesMem::MemoryMgr const MyMemory(ProcId == static_cast<DWORD>(-1) ? 
      GetCurrentProcessId() : ProcId);
        
    // Create manual mapper
    HadesMem::ManualMap const MyManualMapper(MyMemory);
    
    // Inject DLL
    PVOID ModRemote = MyManualMapper.InjectDll(ModulePath, Export);
    
    std::wcout << "Module successfully injected. Base = " << ModRemote << 
      ".\n";
  }
  catch (std::exception const& e)
  {
    std::cerr << boost::diagnostic_information(e) << std::endl;
  }
  
  // Print elapsed time
  std::wcout << "\nElapsed Time: " << ProgTimer.elapsed() << "\n";
  
  return 0;
}
