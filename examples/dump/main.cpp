// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <windows.h>
#include <shellapi.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/region_list.hpp>
#include <hadesmem/process_list.hpp>
#include <hadesmem/process_entry.hpp>

#include "../common/initialize.hpp"

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

  hadesmem::RegionList const regions(&process);
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

void DumpModules(hadesmem::Process const& process)
{
  std::wcout << "\nModules:\n";

  hadesmem::ModuleList const modules(&process);
  for (auto const& module : modules)
  {
    std::wcout << "\n";
    std::wcout << "\tHandle: " << PtrToString(module.GetHandle()) << "\n";
    std::wcout << "\tSize: " << std::hex << module.GetSize() << std::dec 
      << "\n";
    std::wcout << "\tName: " << module.GetName() << "\n";
    std::wcout << "\tPath: " << module.GetPath() << "\n";
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

class CommandLineOpts
{
public:
  CommandLineOpts()
    : help_(false), 
    pid_(0)
  { }

  void SetHelp(bool help)
  {
    help_ = help;
  }

  bool GetHelp() const
  {
    return help_;
  }

  void SetPid(DWORD pid)
  {
    pid_ = pid;
  }

  DWORD GetPid() const
  {
    return pid_;
  }

private:
  bool help_;
  DWORD pid_;
};

CommandLineOpts ParseCommandLine(int argc, wchar_t** argv)
{
  CommandLineOpts opts;

  for (int i = 1; i < argc; ++i)
  {
    std::wstring const current_arg(argv[i]);
    if (current_arg == L"--help")
    {
      opts.SetHelp(true);
    }
    else if (current_arg == L"--pid")
    {
      if (i + 1 < argc)
      {
        std::wstringstream pid_str(argv[i + 1]);
        DWORD pid = 0;
        pid_str >> pid;
        opts.SetPid(pid);
        ++i;
      }
      else
      {
        HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
          hadesmem::ErrorString("Please specify a process ID."));
      }
    }
    else
    {
      HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("Unrecognized argument."));
    }
  }

  return opts;
}

class EnsureLocalFree
{
public:
  explicit EnsureLocalFree(HLOCAL handle)
    : handle_(handle)
  { }

  ~EnsureLocalFree()
  {
    HLOCAL const mem = ::LocalFree(handle_);
    (void)mem;
    assert(!mem);
  }

private:
  EnsureLocalFree(EnsureLocalFree const& other) HADESMEM_DELETED_FUNCTION;
  EnsureLocalFree& operator=(EnsureLocalFree const& other) 
    HADESMEM_DELETED_FUNCTION;

  HLOCAL handle_;
};

}

int main()
{
  try
  {
    DisableUserModeCallbackExceptionFilter();
    EnableCrtDebugFlags();
    EnableTerminationOnHeapCorruption();
    EnableBottomUpRand();

    std::cout << "HadesMem Dumper\n";

    int argc = 0;
    LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLine(), &argc);
    if (!argv)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("CommandLineToArgvW failed.") << 
        hadesmem::ErrorCodeWinLast(last_error));
    }
    EnsureLocalFree ensure_free_command_line(reinterpret_cast<HLOCAL>(argv));

    CommandLineOpts const opts(ParseCommandLine(argc, argv));

    if (opts.GetHelp())
    {
      std::cout << 
        "\nOptions:\n"
        "  --help\t\tproduce help message\n"
        "  --pid arg\t\tprocess id\n";

      return 1;
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

    DWORD const pid = opts.GetPid();

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
    std::cerr << e.what() << "\n";

    return 1;
  }
}
