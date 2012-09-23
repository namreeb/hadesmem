// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/program_options.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "initialize.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/region.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/module_list.hpp"
#include "hadesmem/region_list.hpp"
#include "hadesmem/process_list.hpp"
#include "hadesmem/process_entry.hpp"

std::wstring PtrToString(void const* const ptr)
{
  std::wostringstream str;
  str.imbue(std::locale::classic());
  str << std::hex << reinterpret_cast<DWORD_PTR>(ptr);
  return str.str();
}

int main()
{
  try
  {
    DisableUserModeCallbackExceptionFilter();
    EnableCrtDebugFlags();
    EnableTerminationOnHeapCorruption();
    EnableBottomUpRand();
    ImbueAllDefault();

    std::cout << "HadesMem MemHack\n";

    std::wcout << "\nProcesses:\n";

    hadesmem::ProcessList const processes;
    for (auto const& process_entry : processes)
    {
      std::wcout << "\n";
      std::wcout << "ID: " << process_entry.id << "\n";
      std::wcout << "Threads: " << process_entry.threads << "\n";
      std::wcout << "Parent: " << process_entry.parent << "\n";
      std::wcout << "Priority: " << process_entry.priority << "\n";
      std::wcout << "Name: " << process_entry.name << "\n";

      std::unique_ptr<hadesmem::Process> process;
      try
      {
        process.reset(new hadesmem::Process(process_entry.id));
      }
      catch (...)
      {
        std::wcout << "\nCould not open process for further inspection.\n\n";
        continue;
      }

      std::wcout << "Path: " << hadesmem::GetPath(*process) << "\n";
      std::wcout << "WoW64: " << (hadesmem::IsWoW64(*process) ? "Yes" : "No") 
        << "\n";

      std::wcout << "\nModules:\n";

      hadesmem::ModuleList const modules(&*process);
      for (auto const& module : modules)
      {
        std::wcout << "\n";
        std::wcout << "\tHandle: " << PtrToString(module.GetHandle()) << "\n";
        std::wcout << "\tSize: " << std::hex << module.GetSize() << std::dec 
          << "\n";
        std::wcout << "\tName: " << module.GetName() << "\n";
        std::wcout << "\tPath: " << module.GetPath() << "\n";
      }

      std::wcout << "\nRegions:\n";

      hadesmem::RegionList const regions(&*process);
      for (auto const& region : regions)
      {
        std::wcout << "\n";
        std::wcout << "\tBase: " << PtrToString(region.GetBase()) << "\n";
        std::wcout << "\tAllocation Base: " 
          << PtrToString(region.GetAllocBase()) << "\n";
        std::wcout << "\tAllocation Protect: " << std::hex 
          << region.GetAllocProtect() << std::dec << "\n";
        std::wcout << "\tSize: " << std::hex << region.GetSize() << std::dec 
          << "\n";
        std::wcout << "\tState: " << std::hex << region.GetState() << std::dec 
          << "\n";
        std::wcout << "\tProtect: " << std::hex << region.GetProtect() 
          << std::dec << "\n";
        std::wcout << "\tType: " << std::hex << region.GetType() << std::dec 
          << "\n";
      }
    }
  }
  catch (std::exception const& e)
  {
    std::cerr << "Error!\n";
    std::cerr << boost::diagnostic_information(e) << "\n";

    return 1;
  }
}
