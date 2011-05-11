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
#include <iostream>
#include <exception>
#include <algorithm>

// Boost
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

// Windows
#include <crtdbg.h>
#include <Windows.h>

// Hades
#include "HadesCommon/I18n.hpp"
#include "HadesMemory/Memory.hpp"

int wmain(int argc, wchar_t* argv[], wchar_t* /*envp*/[])
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
    std::wcout << "Hades ExportDiff AMD64 [Version " << VerNum << "]\n";
#elif defined(_M_IX86)
    std::wcout << "Hades ExportDiff IA32 [Version " << VerNum << "]\n";
#else
#error "[HadesMem] Unsupported architecture."
#endif
    std::wcout << "Copyright (C) 2011 RaptorFactor. All rights reserved." << 
      std::endl;
    std::wcout << "Website: http://www.raptorfactor.com/, "
      "Email: raptorfactor@raptorfactor.com." << std::endl;
    std::wcout << "Compiler: \"" << BOOST_COMPILER << "\", Standard "
      "Library: \"" << BOOST_STDLIB << "\", Platform: \"" << BOOST_PLATFORM 
      << "\", Boost: " << BOOST_VERSION << "." << std::endl;
    std::wcout << "Built on " << __DATE__ << " at " << __TIME__ << "." << 
      std::endl << std::endl;
        
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
          if (E.ByName())
          {
            return E.GetName();
          }
          else
          {
            return "#" + boost::lexical_cast<std::string>(E.GetOrdinal());
          }
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
}
