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
#include <vector>
#include <string>
#include <iterator>
#include <iostream>
#include <exception>

// Hades
#include "Hades-Memory/Scripting.h"

bool GetInput(Hades::Memory::ScriptMgr& MyScriptMgr) 
{
  // Prompt for input
  std::wcout << ">";

  // Get command from user
  std::string Input;
  while (!std::getline(std::cin, Input) || Input.empty())
  {
    std::wcout << "Invalid command." << std::endl;
    std::wcout << ">";
  }

  // Check for quit request
  if (Input == "quit")
  {
    return false;
  }

  // Check for runfile request
  if (Input.find(' ') != std::string::npos && Input.substr(0, 
    Input.find(' ')) == "runfile")
  {
    MyScriptMgr.RunFile(Input.substr(Input.find(' ') + 1, 
      std::string::npos));
    return true;
  }

  // Run script
  MyScriptMgr.RunString(Input);

  return true;
}

// Program entry-point.
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
#endif

    // Hades version number
    std::wstring const VerNum(L"TRUNK");

    // Version and copyright output
#if defined(_M_X64)
    std::wcout << "Hades-MemHack AMD64 [Version " << VerNum << "]\n";
#elif defined(_M_IX86)
    std::wcout << "Hades-MemHack IA32 [Version " << VerNum << "]\n";
#else
#error Unsupported platform!
#endif
    std::wcout << "Copyright (C) 2010 Cypherjb. All rights reserved." << 
      std::endl;
    std::wcout << "Website: http://www.cypherjb.com/, "
      "Email: cypher.jb@gmail.com." << std::endl;
    std::wcout << "Built on " << __DATE__ << " at " << __TIME__ << "." << 
      std::endl << std::endl;

    // Create script manager
    Hades::Memory::ScriptMgr MyScriptMgr;

    // If user has passed in a command then run it
    if (argc == 2)
    {
      // Get command
      std::string const Input(boost::lexical_cast<std::string, std::wstring>(
        argv[1]));

      // Check for runfile request
      if (Input.find(' ') != std::string::npos && Input.substr(0, 
        Input.find(' ')) == "runfile")
      {
        MyScriptMgr.RunFile(Input.substr(Input.find(' ') + 1, 
          std::string::npos));
      }
      else
      {
        // Run script
        MyScriptMgr.RunString(Input);
      }
    }
    // Otherwise process commands from user
    else
    {
      for (;;)
      {
        try
        {
          if (!GetInput(MyScriptMgr))
          {
            break;
          }
        }
        catch (boost::exception const& e)
        {
          // Dump error information
          std::cout << boost::diagnostic_information(e);
        }
        catch (std::exception const& e)
        {
          // Dump error information
          std::wcout << "Error! " << e.what() << std::endl;
        }
      }
    }
  }
  catch (boost::exception const& e)
  {
    // Dump error information
    std::cout << boost::diagnostic_information(e);

    // Stop window from automatically closing
    std::wcin.clear();
    std::wcin.sync();
    std::wcin.get();
  }
  catch (std::exception const& e)
  {
    // Dump error information
    std::wcout << "Error! " << e.what() << std::endl;

    // Stop window from automatically closing
    std::wcin.clear();
    std::wcin.sync();
    std::wcin.get();
  }
}
