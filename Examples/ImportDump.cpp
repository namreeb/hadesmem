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
#include <algorithm>

// Boost
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

// Windows
#include <crtdbg.h>
#include <Windows.h>

// Hades
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
    std::wcout << "Hades ImportDump AMD64 [Version " << VerNum << "]\n";
#elif defined(_M_IX86)
    std::wcout << "Hades ImportDump IA32 [Version " << VerNum << "]\n";
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
        
    // Get target process ID from args
    if (argc != 2)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a valid process id."));
    }
    DWORD ProcId = 0;
    try
    {
      ProcId = boost::lexical_cast<DWORD>(argv[1]);
    }
    catch (boost::bad_lexical_cast const& /*e*/)
    {
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("wmain") << 
        Hades::ErrorString("You must supply a valid process id."));
    }
        
    // Open memory manager for self
    Hades::Memory::MemoryMgr MyMemory(ProcId == static_cast<DWORD>(-1) ? 
      GetCurrentProcessId() : ProcId);
    
    // Enumerate module list
    Hades::Memory::ModuleList Modules(MyMemory);
    std::for_each(Modules.begin(), Modules.end(), 
      [&] (Hades::Memory::Module const& M) 
      {
        // Create PE file object for current module
        Hades::Memory::PeFile MyPeFile(MyMemory, M.GetBase());
          
        // Enumerate import dir list for current module
        Hades::Memory::ImportDirList ImpDirs(MyPeFile);
        std::for_each(ImpDirs.begin(), ImpDirs.end(), 
          [&] (Hades::Memory::ImportDir& I)
          {
            // Enumerate import thunk list for current module
            Hades::Memory::ImportThunkList ImpThunks(MyPeFile, 
              I.GetCharacteristics());
            std::for_each(ImpThunks.begin(), ImpThunks.end(), 
              [&] (Hades::Memory::ImportThunk& T)
              {
                // Dump module name, import module, and import name/ordinal
                std::wcout << M.GetName() << " -> ";
                if (T.ByOrdinal())
                {
                  std::cout << boost::format("%s!%d\n") %I.GetName() 
                    %T.GetOrdinal();
                }
                else
                {
                  std::cout << boost::format("%s!%s\n") %I.GetName() 
                    %T.GetName();
                }
              });
          });
      });
  }
  catch (std::exception const& e)
  {
    std::cout << boost::diagnostic_information(e) << std::endl;
  }
}
