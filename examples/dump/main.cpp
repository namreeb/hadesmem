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

// TODO: Dump all TimeDateStamp as actual time-stamps.

// TODO: Warn for unusual time stamps.

// TODO: Support ommiting output for warnings which are not of the specified
// warning type.

// TODO: Move as much corner-case logic as possible into PeLib itself.

// TODO: Check and use the value of the data directory sizes?

// TODO: Add a new 'hostile' warning type for things that are not just
// suspicious, but are actively hostile and never found in 'legitimate' modules,
// like the AOI trick?

// TODO: Handle all tricks and samples from the Corkami PE corpus and the
// Undocumented PECOFF whitepaper by ReversingLabs.

// TODO: Add warnings for cases like that are currently being detected and
// swallowed entirely in PeLib.

// TODO: Refactor out the warning code to operate separately from the dumping
// code where possible.

// TODO: Clean up this tool. It's a disaster of spaghetti code (spaghetti is
// delicious, but we should clean this up anyway...).

// TODO: Add support for warning when a file takes longer than N
// minutes/sections to analyze, with an optional forced timeout.

// TODO: Use setw/setfill/etc when logging in order to get consistently sized
// output.

// TODO: Use backup semantics flags and try to get backup privilege in order to
// make directory enumeration find more files.

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
    WriteNewline(out);
    WriteNormal(out, L"Dumping warned list.", 0);
    for (auto const f : g_all_warned)
    {
      WriteNormal(out, f, 0);
    }
  }
}

void DumpRegions(hadesmem::Process const& process)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNormal(out, L"Regions:", 0);

  hadesmem::RegionList const regions(process);
  for (auto const& region : regions)
  {
    WriteNewline(out);
    WriteNamedHex(
      out, L"Base", reinterpret_cast<std::uintptr_t>(region.GetBase()), 1);
    WriteNamedHex(out,
                  L"Allocation Base",
                  reinterpret_cast<std::uintptr_t>(region.GetAllocBase()),
                  1);
    WriteNamedHex(out, L"Allocation Protect", region.GetAllocProtect(), 1);
    WriteNamedHex(out, L"Size", region.GetSize(), 1);
    WriteNamedHex(out, L"State", region.GetState(), 1);
    WriteNamedHex(out, L"Protect", region.GetProtect(), 1);
    WriteNamedHex(out, L"Type", region.GetType(), 1);
  }
}

void DumpModules(hadesmem::Process const& process)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNormal(out, L"Modules:", 0);

  hadesmem::ModuleList const modules(process);
  for (auto const& module : modules)
  {
    WriteNewline(out);
    WriteNamedHex(
      out, L"Handle", reinterpret_cast<std::uintptr_t>(module.GetHandle()), 1);
    WriteNamedHex(out, L"Size", module.GetSize(), 1);
    WriteNamedNormal(out, L"Name", module.GetName(), 1);
    WriteNamedNormal(out, L"Path", module.GetPath(), 1);

    hadesmem::PeFile const pe_file(
      process, module.GetHandle(), hadesmem::PeFileType::Image, 0);

    try
    {
      hadesmem::DosHeader const dos_header(process, pe_file);
      hadesmem::NtHeaders const nt_headers(process, pe_file);
    }
    catch (std::exception const& /*e*/)
    {
      WriteNewline(out);
      WriteNormal(out, L"WARNING! Not a valid PE file or architecture.", 1);
      g_warned = true;
      continue;
    }

    DumpPeFile(process, pe_file, module.GetPath());
  }
}

void DumpThreadEntry(hadesmem::ThreadEntry const& thread_entry)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNamedHex(out, L"Usage", thread_entry.GetUsage(), 0);
  WriteNamedHex(out, L"ID", thread_entry.GetId(), 0);
  WriteNamedHex(out, L"Owner ID", thread_entry.GetOwnerId(), 0);
  WriteNamedHex(out, L"Base Priority", thread_entry.GetBasePriority(), 0);
  WriteNamedHex(out, L"Delta Priority", thread_entry.GetDeltaPriority(), 0);
  WriteNamedHex(out, L"Flags", thread_entry.GetFlags(), 0);
}

void DumpThreads(DWORD pid)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNormal(out, L"Threads:", 0);

  hadesmem::ThreadList threads(pid);
  for (auto const& thread_entry : threads)
  {
    DumpThreadEntry(thread_entry);
  }
}

void DumpProcessEntry(hadesmem::ProcessEntry const& process_entry)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNamedHex(out, L"ID", process_entry.GetId(), 0);
  WriteNamedHex(out, L"Threads", process_entry.GetThreads(), 0);
  WriteNamedHex(out, L"Parent", process_entry.GetParentId(), 0);
  WriteNamedHex(out, L"Priority", process_entry.GetPriority(), 0);
  WriteNamedNormal(out, L"Normal", process_entry.GetName(), 0);

  DumpThreads(process_entry.GetId());

  std::unique_ptr<hadesmem::Process> process;
  try
  {
    process = std::make_unique<hadesmem::Process>(process_entry.GetId());
  }
  catch (std::exception const& /*e*/)
  {
    WriteNewline(out);
    WriteNormal(out, L"Could not open process for further inspection.", 0);
    WriteNewline(out);
    return;
  }

  // Using the Win32 API to get a processes path can fail for 'zombie'
  // processes. (QueryFullProcessImageName fails with ERROR_GEN_FAILURE.)
  // TODO: Remove this once GetPath is fixed.
  try
  {
    WriteNewline(out);
    WriteNormal(out, L"Path (Win32): " + hadesmem::GetPath(*process), 0);
  }
  catch (std::exception const& /*e*/)
  {
    WriteNewline(out);
    WriteNormal(out, L"WARNING! Could not get Win32 path to process.", 0);
  }
  WriteNormal(out, L"Path (NT): " + hadesmem::GetPathNative(*process), 0);
  WriteNormal(out,
              L"WoW64: " +
                std::wstring(hadesmem::IsWoW64(*process) ? L"Yes" : L"No"),
              0);

  DumpModules(*process);

  DumpRegions(*process);
}

void DumpProcesses()
{
  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNormal(out, L"Processes:", 0);

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
