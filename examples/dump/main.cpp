// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "main.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/program_options.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/debug_privilege.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/process_helpers.hpp>
#include <hadesmem/process_list.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/region_list.hpp>
#include <hadesmem/thread_list.hpp>
#include <hadesmem/thread_entry.hpp>

#include "exports.hpp"
#include "filesystem.hpp"
#include "headers.hpp"
#include "imports.hpp"
#include "sections.hpp"
#include "tls.hpp"

// TODO: Add functionality to make this tool more useful for reversing, such as
// basic heuristics to detect suspicious files, packer/container detection,
// diassembling the EP, compiler detection, dumping more of the file format
// (requires PeLib work), .NET/VB6/etc detection, hashing, etc.

// TODO: Detect and handle tricks like TLS AOI, virtual terminator, etc.

// TODO: Relax some current warnings which are firing for legitimate cases (like
// bound imports causing warnings due to there no longer being a valid ordinal
// or name RVA).

// TODO: Add new warnings for strange cases which require more investigation.
// This includes both new cases, and also existing cases which are currently
// being ignored (including those ignored inside PeLib itself, like a lot of the
// corner cases in RvaToVa). Examples include a virtual or null EP, invalid
// number of data dirs, unknown DOS stub, strange RVAs which lie outside of the
// image or inside the headers, non-standard alignments, no sections, more than
// 96 sections, etc. Add some extra consistentcy checking to help detect strange
// PE files.

// TODO: Investigate places where we have a try/catch because it's probably a
// hack rather then the 'correct' solution. Fix or document all cases.

// TODO: Improve this tool against edge cases like those in the Corkami PE
// corpus. First step is to check each trick and document it if it's already
// handled, or add a note if it's not yet handled. Also add documentation for
// tricks from the ReversingLabs "Undocumented PECOFF" whitepaper.

// TODO: Multi-threading support.

// TODO: Add helper functions such as "HasExportDir" etc. to avoid the
// unnecessary memory allocations and exception handling.

// TODO: Dump string representation of data where possible, such as bitmasks
// (Charateristics etc.), data dir names, etc.

// TODO: Use hex numbers everywhere to simplify the code? (e.g. Ordinals etc.)

// TODO: Dump all TimeDateStamp as actual time-stamps.

// TODO: Warn for unusual time stamps.

// TODO: Support ommiting output for warnings which are not of the specified
// warning type.

// TODO: Move as much corner-case logic as possible into PeLib itself.

// TODO: Check and use the value of the data directory sizes?

// TODO: Test under Application Verifier to try and help find bugs where
// things are virtually terminated or we're reading outside our buffer.
// Currently we're pushing on an extra zero as a hacky workaround for some
// scenarios (like a virtually terminated string).

// TODO: Add a new 'hostile' warning type for things that are not just
// suspicious, but are actively hostile and never found in 'legitimate' modules,
// like the AOI trick?

// TODO: Handle all tricks and samples from the Corkami PE corpus and the
// Undocumented PECOFF whitepaper by ReversingLabs.

// TODO: Add warnings for cases like that are currently being detected and
// swallowed entirely in PeLib.

// TODO: Refactor out the warning code to operate separately from the dumping
// code where possible, without sacrificing too much performance.

// TODO: Clean up this tool. It's a disaster of spaghetti code (spaghetti is
// delicious, but we should clean this up anyway...).

// TODO: Add support for warning when a file takes longer than N
// minutes/sections to analyze, with an optional forced-timeout.

// TODO: Use setw/setfill/etc when logging in order to get consistently sized
// output.

namespace
{

// Record all modules (on disk) which cause a warning when dumped, to make it
// easier to isolate files which require further investigation.
// TODO: Clean this up.
bool g_warned = false;
bool g_warned_enabled = false;
bool g_warned_dynamic = false;
std::vector<std::wstring> g_all_warned;
std::wstring g_warned_file_path;
WarningType g_warned_type = WarningType::kAll;
std::wstring g_current_file_path;

void HandleWarnings(std::wstring const& path)
{
  if (g_warned_enabled && g_warned)
  {
    if (g_warned_dynamic)
    {
      std::unique_ptr<std::wfstream> warned_file_ptr(
        hadesmem::detail::OpenFileWide(g_warned_file_path,
                                       std::ios::out | std::ios::app));
      std::wfstream& warned_file = *warned_file_ptr;
      if (!warned_file)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error() << hadesmem::ErrorString(
                                 "Failed to open warned file for output."));
      }
      warned_file << path << '\n';
    }
    else
    {
      g_all_warned.push_back(path);
    }
  }
}

void DumpWarned(std::wostream& out)
{
  if (!g_all_warned.empty())
  {
    std::wcout << "\nDumping warned list.\n";
    for (auto const f : g_all_warned)
    {
      out << f << "\n";
    }
  }
}

void DumpRegions(hadesmem::Process const& process)
{
  std::wcout << "\nRegions:\n";

  hadesmem::RegionList const regions(process);
  for (auto const& region : regions)
  {
    std::wcout << "\n";
    std::wcout << "\tBase: "
               << hadesmem::detail::PtrToHexString(region.GetBase()) << "\n";
    std::wcout << "\tAllocation Base: " << hadesmem::detail::PtrToHexString(
                                             region.GetAllocBase()) << "\n";
    std::wcout << "\tAllocation Protect: " << std::hex
               << region.GetAllocProtect() << std::dec << "\n";
    std::wcout << "\tSize: " << std::hex << region.GetSize() << std::dec
               << "\n";
    std::wcout << "\tState: " << std::hex << region.GetState() << std::dec
               << "\n";
    std::wcout << "\tProtect: " << std::hex << region.GetProtect() << std::dec
               << "\n";
    std::wcout << "\tType: " << std::hex << region.GetType() << std::dec
               << "\n";
  }
}

void DumpModules(hadesmem::Process const& process)
{
  std::wcout << "\nModules:\n";

  hadesmem::ModuleList const modules(process);
  for (auto const& module : modules)
  {
    std::wcout << "\n";
    std::wcout << "\tHandle: "
               << hadesmem::detail::PtrToHexString(module.GetHandle()) << "\n";
    std::wcout << "\tSize: " << std::hex << module.GetSize() << std::dec
               << "\n";
    std::wcout << "\tName: " << module.GetName() << "\n";
    std::wcout << "\tPath: " << module.GetPath() << "\n";

    hadesmem::PeFile const pe_file(
      process, module.GetHandle(), hadesmem::PeFileType::Image, 0);

    try
    {
      hadesmem::DosHeader const dos_header(process, pe_file);
      hadesmem::NtHeaders const nt_headers(process, pe_file);
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\n";
      std::wcout << "\tWARNING! Not a valid PE file or architecture.\n";
      g_warned = true;
      continue;
    }

    DumpPeFile(process, pe_file, module.GetPath());
  }
}

void DumpThreadEntry(hadesmem::ThreadEntry const& thread_entry)
{
  std::wcout << "\n";
  std::wcout << "Usage: " << thread_entry.GetUsage() << "\n";
  std::wcout << "ID: " << thread_entry.GetId() << "\n";
  std::wcout << "Owner ID: " << thread_entry.GetOwnerId() << "\n";
  std::wcout << "Base Priority: " << thread_entry.GetBasePriority() << "\n";
  std::wcout << "Delta Priority: " << thread_entry.GetDeltaPriority() << "\n";
  std::wcout << "Flags: " << thread_entry.GetFlags() << "\n";
}

void DumpThreads(DWORD pid)
{
  std::wcout << "\nThreads:\n";

  hadesmem::ThreadList threads(pid);
  for (auto const& thread_entry : threads)
  {
    DumpThreadEntry(thread_entry);
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

  DumpThreads(process_entry.GetId());

  std::unique_ptr<hadesmem::Process> process;
  try
  {
    process = std::make_unique<hadesmem::Process>(process_entry.GetId());
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\nCould not open process for further inspection.\n\n";
    return;
  }

  // Using the Win32 API to get a processes path can fail for 'zombie'
  // processes. (QueryFullProcessImageName fails with ERROR_GEN_FAILURE.)
  // TODO: Remove this once GetPath is fixed.
  try
  {
    std::wcout << "\nPath (Win32): " << hadesmem::GetPath(*process) << "\n";
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\nWARNING! Could not get Win32 path to process.\n";
  }
  std::wcout << "Path (NT): " << hadesmem::GetPathNative(*process) << "\n";
  std::wcout << "WoW64: " << (hadesmem::IsWoW64(*process) ? "Yes" : "No")
             << "\n";

  DumpModules(*process);

  DumpRegions(*process);
}

void DumpProcesses()
{
  std::wcout << "\nProcesses:\n";

  hadesmem::ProcessList const processes;
  for (auto const& process_entry : processes)
  {
    DumpProcessEntry(process_entry);
  }
}
}

void DumpPeFile(hadesmem::Process const& process,
                hadesmem::PeFile const& pe_file,
                std::wstring const& path)
{
  ClearWarnForCurrentFile();

  DumpHeaders(process, pe_file);

  DumpSections(process, pe_file);

  DumpTls(process, pe_file);

  DumpExports(process, pe_file);

  // TODO: Fix the app/library so this is no longer necessary... Should the
  // bound import dumper simply perform an extra validation pass on the import
  // dir? What about perf? Needs more investigation.
  bool has_new_bound_imports_any = false;
  DumpImports(process, pe_file, has_new_bound_imports_any);

  DumpBoundImports(process, pe_file, has_new_bound_imports_any);

  HandleWarnings(path);
}

void SetCurrentFilePath(std::wstring const& path)
{
  g_current_file_path = path;
}

void WarnForCurrentFile(WarningType warned_type)
{
  if (warned_type == g_warned_type || g_warned_type == WarningType::kAll)
  {
    g_warned = true;
  }
}

void ClearWarnForCurrentFile()
{
  g_warned = false;
}

int main(int /*argc*/, char * /*argv*/ [])
{
  try
  {
    std::cout << "HadesMem Dumper [" << HADESMEM_VERSION_STRING << "]\n";

    boost::program_options::options_description opts_desc("General options");
    opts_desc.add_options()("help", "produce help message")(
      "pid", boost::program_options::value<DWORD>(), "target process id")(
      "file", boost::program_options::wvalue<std::wstring>(), "target file")(
      "dir", boost::program_options::wvalue<std::wstring>(), "target dir")(
      "warned", "dump list of files which cause warnings")(
      "warned-file",
      boost::program_options::wvalue<std::wstring>(),
      "dump warned list to file instead of stdout")(
      "warned-file-dynamic",
      "dump warnings to file on the fly rather than at the end")(
      "warned-type",
      boost::program_options::wvalue<int>(),
      "filter warned file using warned type");

    std::vector<std::wstring> const args =
      boost::program_options::split_winmain(::GetCommandLine());
    boost::program_options::variables_map var_map;
    boost::program_options::store(
      boost::program_options::wcommand_line_parser(args)
        .options(opts_desc)
        .run(),
      var_map);
    boost::program_options::notify(var_map);

    if (var_map.count("help"))
    {
      std::cout << '\n' << opts_desc << '\n';
      return 1;
    }

    g_warned_enabled = !!var_map.count("warned");
    g_warned_dynamic = !!var_map.count("warned-file-dynamic");
    if (var_map.count("warned-file"))
    {
      g_warned_file_path = var_map["warned-file"].as<std::wstring>();
    }
    else
    {
      if (g_warned_dynamic)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error()
          << hadesmem::ErrorString(
               "Please specify a file path for dynamic warnings."));
      }
    }
    if (var_map.count("warned-type"))
    {
      int const warned_type = var_map["warned-type"].as<int>();
      switch (warned_type)
      {
      case static_cast<int>(WarningType::kSuspicious) :
        g_warned_type = WarningType::kSuspicious;
        break;
      case static_cast<int>(WarningType::kUnsupported) :
        g_warned_type = WarningType::kUnsupported;
        break;
      default:
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error() << hadesmem::ErrorString("Unknown warned type."));
        break;
      }
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

    if (var_map.count("pid"))
    {
      DWORD pid = var_map["pid"].as<DWORD>();

      hadesmem::ProcessList const processes;
      auto iter =
        std::find_if(std::begin(processes),
                     std::end(processes),
                     [pid](hadesmem::ProcessEntry const& process_entry)
      { return process_entry.GetId() == pid; });
      if (iter != std::end(processes))
      {
        DumpProcessEntry(*iter);
      }
      else
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error() << hadesmem::ErrorString(
                                 "Failed to find requested process."));
      }
    }
    else if (var_map.count("file"))
    {
      DumpFile(var_map["file"].as<std::wstring>());
    }
    else if (var_map.count("dir"))
    {
      DumpDir(var_map["dir"].as<std::wstring>());
    }
    else
    {
      DumpThreads(static_cast<DWORD>(-1));

      DumpProcesses();

      std::wcout << "\nFiles:\n";

      std::wstring const self_path = hadesmem::detail::GetSelfPath();
      std::wstring const root_path = hadesmem::detail::GetRootPath(self_path);
      DumpDir(root_path);
    }

    if (g_warned_enabled)
    {
      if (var_map.count("warned-file") && !var_map.count("warned-file-dynamic"))
      {
        std::wstring const warned_file_path =
          var_map["warned-file"].as<std::wstring>();
        std::unique_ptr<std::wfstream> warned_file_ptr(
          hadesmem::detail::OpenFileWide(warned_file_path, std::ios::out));
        std::wfstream& warned_file = *warned_file_ptr;
        if (!warned_file)
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(
            hadesmem::Error() << hadesmem::ErrorString(
                                   "Failed to open warned file for output."));
        }
        DumpWarned(warned_file);
      }
      else
      {
        DumpWarned(std::wcout);
      }
    }

    return 0;
  }
  catch (...)
  {
    std::cerr << "\nError!\n"
              << boost::current_exception_diagnostic_information() << '\n';

    // TODO: Clean up this hack.
    if (!g_current_file_path.empty())
    {
      std::wcerr << "\nCurrent file: " << g_current_file_path << "\n";
    }

    return 1;
  }
}
