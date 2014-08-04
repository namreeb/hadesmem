// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <tclap/cmdline.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/debug_privilege.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_helpers.hpp>

#include "character_manager.hpp"
#include "translated_string_repository.hpp"

int main(int argc, char* argv[])
{
  try
  {
    std::cout << "HadesMem \"Divinity: Original Sin\" Trainer ["
              << HADESMEM_VERSION_STRING << "]\n";

    TCLAP::CmdLine cmd{
      "Divinity: Original Sin Trainer", ' ', HADESMEM_VERSION_STRING};
    TCLAP::ValueArg<DWORD> pid_arg{"",
                                   "pid",
                                   "Process ID (for multiple ESO instances)",
                                   false,
                                   0,
                                   "DWORD",
                                   cmd};
    cmd.parse(argc, argv);

    try
    {
      hadesmem::GetSeDebugPrivilege();

      std::wcout << "\nAcquired SeDebugPrivilege.\n";
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\nFailed to acquire SeDebugPrivilege.\n";
    }

    std::unique_ptr<hadesmem::Process> process;

    if (pid_arg.isSet())
    {
      DWORD const pid = pid_arg.getValue();
      process = std::make_unique<hadesmem::Process>(pid);
    }
    else
    {
      std::wstring const kProcName = L"EoCApp.exe";
      process = std::make_unique<hadesmem::Process>(
        hadesmem::GetProcessByName(kProcName, false));
    }

    auto const base = reinterpret_cast<std::uint8_t*>(
      hadesmem::Module(*process, nullptr).GetHandle());

    std::cout << '\n';

    DumpCharacterManager(*process, base);

    DumpStringRepository(*process, base);

    std::cout << "\nFinished.\n";

    return 0;
  }
  catch (...)
  {
    std::cerr << "\nError!\n";
    std::cerr << boost::current_exception_diagnostic_information() << '\n';

    return 1;
  }
}
