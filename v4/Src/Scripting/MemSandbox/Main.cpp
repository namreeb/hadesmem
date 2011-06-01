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
#include <tchar.h>
#include <crtdbg.h>
#include <Windows.h>

// C++ Standard Library
#include <limits>
#include <vector>
#include <string>
#include <iterator>
#include <iostream>
#include <exception>

// Boost
#pragma warning(push, 1)
#include <boost/timer.hpp>
#include <boost/python.hpp>
#include <boost/config.hpp>
#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#pragma warning(pop)

// Hades
#include "HadesMemory/Memory.hpp"

bool GetInput(boost::python::object const& PythonNamespace) 
{
  // Prompt for input
  std::cout << ">";

  // Get command from user
  std::string Input;
  while (!std::getline(std::cin, Input) || Input.empty())
  {
    std::cout << "Invalid command." << std::endl;
    std::cout << ">";

    // Reset input stream if necessary
    if (!std::cin)
    {
      std::cin.clear();
      std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    }
  }

  // Check for quit request
  if (Input == "quit" || Input == "exit")
  {
    return false;
  }

  try
  {
    // Run script
    boost::python::exec(Input.c_str(), PythonNamespace, PythonNamespace);
  }
  catch (...)
  {
    // Handle exceptions
    boost::python::handle_exception();

    // Print error string
    PyErr_Print();
  }

  return true;
}

// Program entry-point.
int wmain(int argc, wchar_t *argv[ ], wchar_t* /*envp*/[])
{
  // Program timer
  boost::timer ProgTimer;

  try
  {
    // Attempt to detect memory leaks in debug mode
#ifdef _DEBUG
    int CurrentFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    int NewFlags = (_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF | 
      _CRTDBG_CHECK_ALWAYS_DF);
    _CrtSetDbgFlag(CurrentFlags | NewFlags);
#endif

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

    // Hades version number
    std::wstring const VerNum(L"TRUNK");

    // Version and copyright output
#if defined(_M_X64)
    std::wcout << "Hades MemSandbox AMD64 [Version " << VerNum << "]\n";
#elif defined(_M_IX86)
    std::wcout << "Hades MemSandbox IA32 [Version " << VerNum << "]\n";
#else
#error "[HadesMem] Unsupported architecture."
#endif
    std::wcout << "Copyright (C) 2010 RaptorFactor. All rights reserved." << 
      std::endl;
    std::wcout << "Website: http://www.raptorfactor.com/, "
      "Email: raptorfactor@raptorfactor.com." << std::endl;
    std::wcout << "Compiler: \"" << BOOST_COMPILER << "\", Standard "
      "Library: \"" << BOOST_STDLIB << "\", Platform: \"" << BOOST_PLATFORM 
      << "\", Boost: " << BOOST_VERSION << "." << std::endl;
    std::wcout << "Built on " << __DATE__ << " at " << __TIME__ << "." << 
      std::endl << std::endl;

    // Auto-close flag (Set by Boost.ProgramOptions)
    bool KeepOpen = false;
    // Path to script file (Set by Boost.ProgramOptions)
    std::wstring FilePath;
    // Script string (Set by Boost.ProgramOptions)
    std::string ScriptStr;

    // Set program option descriptions
    boost::program_options::options_description OptsDesc("Allowed options");
    OptsDesc.add_options()
      ("help", "display help")
      ("keep-open", boost::program_options::wvalue<bool>(&KeepOpen)->
        zero_tokens(), "keep console window open")
      ("file", boost::program_options::wvalue<std::wstring>(&FilePath), 
        "file to execute")
      ("string", boost::program_options::value<std::string>(&ScriptStr), 
        "string to execute")
      ;

    // Parse program options
    boost::program_options::variables_map Opts;
    boost::program_options::store(boost::program_options::parse_command_line(
      argc, argv, OptsDesc), Opts);
    boost::program_options::notify(Opts);

    // Print help if requested
    if (Opts.count("help")) 
    {
      // Print help
      std::cout << OptsDesc << std::endl;

      // Stop window from automatically closing if required
      if (KeepOpen)
      {
        std::wcin.clear();
        std::wcin.sync();
        std::wcin.get();
      }

      // Quit
      return 1;
    }

    // Initialize Python
    Py_Initialize();

    // Debug output
    std::wcout << "Python " << Py_GetVersion() << "\n" << std::endl;

    // Retrieve the main module.
    boost::python::object PythonMain(boost::python::import("__main__"));

    // Retrieve the main module's namespace
    boost::python::object PythonGlobal(PythonMain.attr("__dict__"));

    try
    {
      // Import PyHadesMem
      boost::python::exec("import PyHadesMem", PythonGlobal, PythonGlobal);
    }
    catch (...)
    {
      // Print error string
      PyErr_Print();

      // Throw exception
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("_tmain") << 
        Hades::ErrorString("Failed to load PyHadesMem."));
    }

    // If user has passed in a file-name then run it
    if (!FilePath.empty())
    {
      // Get file
      boost::filesystem::path const FilePathReal(FilePath);
      if (!boost::filesystem::exists(FilePathReal))
      {
        BOOST_THROW_EXCEPTION(Hades::HadesError() << 
          Hades::ErrorFunction("_tmain") << 
          Hades::ErrorString("Requested file could not be found."));
      }

      try
      {
        // Run file
        boost::python::exec_file(FilePathReal.string().c_str(), PythonGlobal, 
          PythonGlobal);
      }
      catch (...)
      {
        // Handle exceptions
        boost::python::handle_exception();

        // Print error string
        PyErr_Print();
      }

    }
    // If user has passed in a string then run it
    else if (!ScriptStr.empty())
    {
      try
      {
        // Run script
        boost::python::exec(ScriptStr.c_str(), PythonGlobal, PythonGlobal);
      }
      catch (...)
      {
        // Handle exceptions
        boost::python::handle_exception();

        // Print error string
        PyErr_Print();
      }
    }
    // Otherwise process commands from user
    else
    {
      for (;;)
      {
        try
        {
          if (!GetInput(PythonGlobal))
          {
            break;
          }
        }
        catch (std::exception const& e)
        {
          // Dump error information
          std::cout << boost::diagnostic_information(e);
        }
      }
    }

    // Print elapsed time
    std::wcout << "\nElapsed Time: " << ProgTimer.elapsed() << "." << 
      std::endl;

    // Stop window from automatically closing if required
    if (KeepOpen)
    {
      std::wcin.clear();
      std::wcin.sync();
      std::wcin.get();
    }
  }
  catch (std::exception const& e)
  {
    // Dump error information
    std::cout << boost::diagnostic_information(e);

    // Print elapsed time
    std::wcout << "\nElapsed Time: " << ProgTimer.elapsed() << "." << 
      std::endl;

    // Always keep window open in case of an error
    std::wcin.clear();
    std::wcin.sync();
    std::wcin.get();
  }
}
