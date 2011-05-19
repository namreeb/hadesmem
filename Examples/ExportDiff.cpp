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
#include <set>
#include <string>
#include <iterator>
#include <iostream>
#include <exception>
#include <algorithm>

// Boost
#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

// Windows
#include <Windows.h>
#include <Shellapi.h>

// Hades
#include "HadesMemory/Memory.hpp"
#include "HadesCommon/EnsureCleanup.hpp"

#ifdef HADES_GCC
int wmain(int argc, wchar_t* argv[]);

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
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

int wmain(int argc, wchar_t* argv[])
{
  try
  {
    // Attempt to detect memory leaks in debug mode
#ifdef _DEBUG
    int CurrentFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    int NewFlags = (_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | 
      _CRTDBG_CHECK_ALWAYS_DF);
    _CrtSetDbgFlag(CurrentFlags | NewFlags);

    // Get default heap
    HANDLE ProcHeap = GetProcessHeap();
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
    std::wcout << "Hades PeFile AMD64 [Version " << VerNum << "]\n";
#elif defined(_M_IX86)
    std::wcout << "Hades PeFile IA32 [Version " << VerNum << "]\n";
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
    
    // Get target module from args
    if (argc < 2)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a base module path."));
    }
    boost::filesystem::path BaseModulePath;
    try
    {
      BaseModulePath = argv[1];
    }
    catch (std::exception const& /*e*/)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a base module path."));
    }
    
    // Get target module from args
    if (argc < 3)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a target module path."));
    }
    boost::filesystem::path TargetModulePath;
    try
    {
      TargetModulePath = argv[2];
    }
    catch (std::exception const& /*e*/)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a target module path."));
    }
        
    // Open memory manager
    Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
    
    // Load base module
    Hades::Windows::EnsureFreeLibrary BaseMod = LoadLibraryEx(
      BaseModulePath.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!BaseMod)
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("Could not load base module.") << 
        Hades::ErrorCode(LastError));
    }
    
    // Load target module
    Hades::Windows::EnsureFreeLibrary TargetMod = LoadLibraryEx(
      TargetModulePath.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!TargetMod)
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("Could not load base module.") << 
        Hades::ErrorCode(LastError));
    }
    
    // Get exports for module
    auto GetExports = [&MyMemory] (HMODULE Base) -> std::set<std::string> 
    {
      // Create PE file object for current module
      Hades::Memory::PeFile MyPeFile(MyMemory, Base);
        
      // Export list
      std::set<std::string> Result;
        
      Hades::Memory::ExportList Exports(MyPeFile);
      std::transform(Exports.begin(), Exports.end(), std::inserter(Result, 
        Result.begin()), 
        [&] (Hades::Memory::Export& E) -> std::string 
        {
          std::string Result;
          
          if (E.ByName())
          {
            Result += E.GetName();
          }
          else
          {
            Result += "#" + boost::lexical_cast<std::string>(E.GetOrdinal());
          }
          
          // Note: Was causing some 'false positives' to be raised.
          //if (E.Forwarded())
          //{
          //  Result += " -> " + E.GetForwarder();
          //}
          
          return Result;
        });
        
      return Result;
    };
    
    // Get exports for base module
    std::set<std::string> BaseExports(GetExports(BaseMod));
      
    // Get exports for target module
    std::set<std::string> TargetExports(GetExports(TargetMod));
    
    // Find exports in target that do not exist in base
    std::set<std::string> NewExports;
    set_difference(TargetExports.begin(), TargetExports.end(), 
      BaseExports.begin(), BaseExports.end(), std::inserter(NewExports, 
      NewExports.begin()));
      
    // Dump new exports
    std::wcout << "New Exports:\n";
    std::copy(NewExports.begin(), NewExports.end(), 
      std::ostream_iterator<std::string>(std::cout, "\n"));
  }
  catch (std::exception const& e)
  {
    std::cerr << boost::diagnostic_information(e) << std::endl;
  }
  
  return 0;
}