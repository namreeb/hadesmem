// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/program_options.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/region_list.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/process_list.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/detail/initialize.hpp>
#include <hadesmem/pelib/export_list.hpp>
#include <hadesmem/pelib/import_thunk.hpp>
#include <hadesmem/pelib/import_dir_list.hpp>
#include <hadesmem/pelib/import_thunk_list.hpp>

namespace
{

std::wstring PtrToString(void const* const ptr)
{
  std::wostringstream str;
  str.imbue(std::locale::classic());
  str << std::hex << reinterpret_cast<DWORD_PTR>(ptr);
  return str.str();
}

void DumpRegions(hadesmem::Process const& process)
{
  std::wcout << "\nRegions:\n";

  hadesmem::RegionList const regions(process);
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

void DumpExports(hadesmem::Process const& process, 
  hadesmem::Module const& module)
{
  std::wcout << "\n\tExports:\n";

  hadesmem::PeFile pe_file(process, module.GetHandle(), 
    hadesmem::PeFileType::Image);
  hadesmem::ExportList exports(process, pe_file);
  for (auto const& e : exports)
  {
    std::wcout << std::boolalpha;
    std::wcout << "\n";
    std::wcout << "\t\tRVA: " << std::hex << e.GetRva() << std::dec << "\n";
    std::wcout << "\t\tVA: " << PtrToString(e.GetVa()) << "\n";
    std::wcout << "\t\tName: " << e.GetName().c_str() << "\n";
    std::wcout << "\t\tOrdinal: " << e.GetOrdinal() << "\n";
    std::wcout << "\t\tByName: " << e.ByName() << "\n";
    std::wcout << "\t\tByOrdinal: " << e.ByOrdinal() << "\n";
    std::wcout << "\t\tIsForwarded: " << e.IsForwarded() << "\n";
    std::wcout << "\t\tForwarder: " << e.GetForwarder().c_str() << "\n";
    std::wcout << "\t\tForwarderModule: " << e.GetForwarderModule().c_str() << 
      "\n";
    std::wcout << "\t\tForwarderFunction: " << 
      e.GetForwarderFunction().c_str() << "\n";
    std::wcout << "\t\tIsForwardedByOrdinal: " << e.IsForwardedByOrdinal() << 
      "\n";
    if (e.IsForwardedByOrdinal())
    {
      try
      {
        std::wcout << "\t\tForwarderOrdinal: " << e.GetForwarderOrdinal() << 
          "\n";
      }
      catch (std::exception const& /*e*/)
      {
        std::wcout << "\t\tForwarderOrdinal Invalid.\n";
      }
    }
    std::wcout << std::noboolalpha;
  }
}

void DumpImports(hadesmem::Process const& process, 
  hadesmem::Module const& module)
{
  std::wcout << "\n\tImport Dirs:\n";

  hadesmem::PeFile pe_file(process, module.GetHandle(), 
    hadesmem::PeFileType::Image);
  hadesmem::ImportDirList import_dirs(process, pe_file);
  for (auto const& dir : import_dirs)
  {
    std::wcout << std::boolalpha;

    std::wcout << "\n";
    std::wcout << "\t\tCharacteristics: " << std::hex << dir.GetCharacteristics() << std::dec << "\n";
    std::wcout << "\t\tTimeDateStamp: " << dir.GetTimeDateStamp() << "\n";
    std::wcout << "\t\tForwarderChain: " << dir.GetForwarderChain() << "\n";
    std::wcout << "\t\tName (Raw): " << std::hex << dir.GetNameRaw() << std::dec << "\n";
    std::wcout << "\t\tName: " << dir.GetName().c_str() << "\n";
    std::wcout << "\t\tFirstThunk: " << dir.GetFirstThunk() << "\n";

    std::wcout << "\n\t\tImport Thunks:\n";

    hadesmem::ImportThunkList import_thunks(process, pe_file, 
      dir.GetCharacteristics());
    for (auto const& thunk : import_thunks)
    {
      std::wcout << "\n";
      std::wcout << "\t\t\tAddressOfData: " << thunk.GetAddressOfData() << "\n";
      std::wcout << "\t\t\tOrdinalRaw: " << thunk.GetOrdinalRaw() << "\n";
      std::wcout << "\t\t\tByOrdinal: " << thunk.ByOrdinal() << "\n";
      if (thunk.ByOrdinal())
      {
        std::wcout << "\t\t\tOrdinal: " << thunk.GetOrdinal() << "\n";
      }
      else
      {
        std::wcout << "\t\t\tHint: " << thunk.GetHint() << "\n";
        std::wcout << "\t\t\tName: " << thunk.GetName().c_str() << "\n";
      }
      std::wcout << "\t\t\tFunction: " << thunk.GetFunction() << "\n";
    }

    std::wcout << std::noboolalpha;
  }
}

void DumpModules(hadesmem::Process const& process)
{
  std::wcout << "\nModules:\n";

  hadesmem::ModuleList const modules(process);
  for (auto const& module : modules)
  {
    std::wcout << "\n";
    std::wcout << "\tHandle: " << PtrToString(module.GetHandle()) << "\n";
    std::wcout << "\tSize: " << std::hex << module.GetSize() << std::dec 
      << "\n";
    std::wcout << "\tName: " << module.GetName() << "\n";
    std::wcout << "\tPath: " << module.GetPath() << "\n";

    DumpExports(process, module);

    DumpImports(process, module);
  }
}

void DumpProcessEntry(hadesmem::ProcessEntry const& process_entry)
{
  std::wcout << "\n";
  std::wcout << "ID: " << process_entry.GetId() << "\n";
  std::wcout << "Threads: " << process_entry.GetThreads() << "\n";
  std::wcout << "Parent: " << process_entry.GetParentId() << "\n";
  std::wcout << "Priority: " << process_entry.GetPriority() << "\n";
  std::wcout << "Name: " << process_entry.GetName() << "\n";

  std::unique_ptr<hadesmem::Process> process;
  try
  {
    process.reset(new hadesmem::Process(process_entry.GetId()));
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\nCould not open process for further inspection.\n\n";
    return;
  }

  std::wcout << "Path: " << hadesmem::GetPath(*process) << "\n";
  std::wcout << "WoW64: " << (hadesmem::IsWoW64(*process) ? "Yes" : "No") 
    << "\n";

  DumpModules(*process);

  DumpRegions(*process);
}

}

int main(int /*argc*/, char* /*argv*/[]) 
{
  try
  {
    hadesmem::detail::DisableUserModeCallbackExceptionFilter();
    hadesmem::detail::EnableCrtDebugFlags();
    hadesmem::detail::EnableTerminationOnHeapCorruption();
    hadesmem::detail::EnableBottomUpRand();
    hadesmem::detail::ImbueAllDefault();

    std::cout << "HadesMem Dumper\n";

    boost::program_options::options_description opts_desc(
      "General options");
    opts_desc.add_options()
      ("help", "produce help message")
      ("pid", boost::program_options::value<DWORD>(), "process id")
      ;

    std::vector<std::wstring> const args = boost::program_options::
      split_winmain(GetCommandLine());
    boost::program_options::variables_map var_map;
    boost::program_options::store(boost::program_options::wcommand_line_parser(
      args).options(opts_desc).run(), var_map);
    boost::program_options::notify(var_map);

    if (var_map.count("help"))
    {
      std::cout << '\n' << opts_desc << '\n';
      return 1;
    }

    DWORD pid = 0;
    if (var_map.count("pid"))
    {
      pid = var_map["pid"].as<DWORD>();
    }

    try
    {
      hadesmem::GetSeDebugPrivilege();

      std::wcout << "\nAcquired SeDebugPrivilege.\n";
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\nFailed to acquire SeDebugPrivilege.\n";
    }

    if (pid)
    {
      hadesmem::ProcessList const processes;
      auto iter = std::find_if(std::begin(processes), std::end(processes), 
        [pid] (hadesmem::ProcessEntry const& process_entry)
        {
          return process_entry.GetId() == pid;
        });
      if (iter != std::end(processes))
      {
        DumpProcessEntry(*iter);
      }
      else
      {
        HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
          hadesmem::ErrorString("Failed to find requested process."));
      }
    }
    else
    {
      std::wcout << "\nProcesses:\n";

      hadesmem::ProcessList const processes;
      for (auto const& process_entry : processes)
      {
        DumpProcessEntry(process_entry);
      }
    }

    return 0;
  }
  catch (std::exception const& e)
  {
    std::cerr << "\nError!\n";
    std::cerr << boost::diagnostic_information(e) << '\n';

    return 1;
  }
}
