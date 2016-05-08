// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#define TCLAP_SETBASE_ZERO 1
#if !defined(_M_X64)
#define HAVE_LONG_LONG
#endif

#include "main.hpp"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>
#include <time.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <tclap/cmdline.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/debug_privilege.hpp>
#include <hadesmem/detail/dump.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/detail/thread_pool.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/dos_header.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/process_helpers.hpp>
#include <hadesmem/process_list.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/region_list.hpp>
#include <hadesmem/thread_list.hpp>
#include <hadesmem/thread_entry.hpp>

#include "bound_imports.hpp"
#include "exports.hpp"
#include "filesystem.hpp"
#include "headers.hpp"
#include "imports.hpp"
#include "overlay.hpp"
#include "print.hpp"
#include "relocations.hpp"
#include "sections.hpp"
#include "strings.hpp"
#include "tls.hpp"
#include "warning.hpp"

// TODO: Re-add unicode command line support to this and all other apps. TCLAP
// will likely have to be replaced as it only supports narrow strings.
// Powershell ISE is a good way to test this I think. http://goo.gl/zMzyjS

// TODO: Add support for detecting the most expensive to process files in a
// batch. Time how long it takes to fully dump each file, and record the top N
// entries. Be careful not to factor in IO however (i.e. only time once the file
// is copied into our buffer).

// TODO: Add PEID DB support.

// TODO: Implement resources dumping support.
// 	http://www.brokenthorn.com/Resources/OSDevPE.html
// 	http://blogs.msdn.com/b/oldnewthing/archive/2012/07/20/10331787.aspx

// TODO: Implement debug dumping support.

// TODO: Implement .NET dumping support.

// TODO: Move all special cases into main PELib API.

// TODO: Detect/handle all tricks from 'Undocumented PECOFF' whitepaper.

// TODO: Detect/handle all tricks from 'Corkami'.

// TODO: Instead of hardcoding lots of special logic in this tool (especially
// for warnings etc.), we should move to an attribute based system where the
// library sets attributes which can be queried by consumers like this tool.

// TODO: Add a GUI (but continue to support CLI).

// TODO: Add sample files for all the corner cases we're handling, and ensure it
// is correct, so we can add regression tests. This includes logic in PeLib as
// well.

// TODO: Add warnings for cases like that are currently being detected and
// swallowed entirely in PeLib.

// TODO: Dump string representation of data where possible, such as bitmasks
// (Charateristics etc.), data dir names, etc.

// TODO: Add helper functions such as "HasExportDir" etc. to avoid the
// unnecessary memory allocations and exception handling.

// TODO: Check and use the value of the data directory sizes? e.g. To limit
// enumeration of imports and exports. Need to check the loader behaviour to see
// what we're allowed to do though. Size is probably ignored in most cases.

// TODO: Investigate places where we have a try/catch because it's probably a
// hack rather then the 'correct' solution. Fix or document all cases.

// TODO: Fix the app/library so the "has_bound_imports_any" out-param is no
// longer necessary... Should the bound import dumper simply perform an extra
// validation pass on the import dir? What about perf? Needs more investigation.

// TODO: Add a new 'hostile' warning type for things that are not just
// suspicious, but are actively hostile and never found in 'legitimate' modules,
// like the AOI trick.

// TODO: Clean up this tool. It's a disaster of spaghetti code (spaghetti is
// delicious, but we should clean this up anyway...).

// TODO: Add HookShark-style replacement/alternative.

// TODO: Memory-editor, debugger, etc.

// TODO: Add functionality to make this tool more useful for reversing, such as
// basic heuristics to detect suspicious files, packer/container detection,
// diassembling the EP, compiler detection, dumping more of the file format
// (requires PeLib work), .NET/VB6/etc detection, hashing, etc.

// TODO: Fix files where we're currently getting an empty or invalid import
// directory, but we're actually wrong! (Sample: Lab10-01.sys - Practical
// Malware Analysis)

// TODO: Move this and other complex examples to its own repro and solution. The
// examples included with the library should be minimal and designed for
// expository purposes.

// TODO: When logging VAs, adjust them to use ImageBase for a consistent faux-VA
// rather than just printing a pointer to wherever the file happens to lie in
// memory at the time of dumping.

// TODO: Remove all the parts of the constructors for BoundImportDescriptor,
// ImportDescriptor, etc that take a null pointer as the base and figure it out.
// This should be done in the list class instead! The constructor of the actual
// descriptor should just take what it's given and roll with it… Needs more
// thought for corner cases though like the virtual termination, virtual
// overlap, etc.

// TODO: Fix all the hacks where we're currently checking for unnecessary data
// in the constructors. e.g. Bound import descriptor constructor currently
// checks for the presence of an import dir. Should we really be doing that
// there?

// TODO: Detect and dump code caves (similar to PeStudio).

// TODO: Add entropy dumping (sections, EP section, resources, overall file,
// etc.).

// TODO: Add support for a timeout. Especially important when doing a batch
// analysis (e.g. running on an entire drive) so we can ensure it doesn't take
// forever, and quickly identify those files which are (probably) being
// mishandled.

// TODO: Warn on files without a DOS stub as it probably means they're
// hand-crafted?

// TODO: Add support for dumping arbitrary memory blobs using updated DumpMemory
// API in detail/dump.hpp.

namespace
{
// TODO: Clean up this hack (and global state).
thread_local std::wstring g_current_file_path;

// TODO: Clean up global state.
bool g_quiet = false;
bool g_strings = false;
std::uint32_t g_flags = hadesmem::detail::DumpFlags::kNone;
DWORD g_oep = 0;
std::wstring g_module_name;
std::uintptr_t g_module_base = 0;
std::uintptr_t g_raw_base = 0;
std::size_t g_raw_size = 0;

template <typename CharT>
class QuietStreamBuf : public std::basic_streambuf<CharT>
{
public:
  std::streamsize xsputn(const char_type* /*s*/, std::streamsize n) override
  {
    return n;
  }

  int_type overflow(int_type /*c*/) override
  {
    return 1;
  }
};

void DumpRegions(hadesmem::Process const& process)
{
  std::wostream& out = GetOutputStreamW();

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
  std::wostream& out = GetOutputStreamW();

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
      process, module.GetHandle(), hadesmem::PeFileType::kImage, 0);

    try
    {
      hadesmem::DosHeader const dos_header(process, pe_file);
      hadesmem::NtHeaders const nt_headers(process, pe_file);
    }
    catch (std::exception const& /*e*/)
    {
      WriteNewline(out);
      WriteNormal(out, L"WARNING! Not a valid PE file or architecture.", 1);
      continue;
    }

    DumpPeFile(process, pe_file, module.GetPath());
  }
}

void DumpThreadEntry(hadesmem::ThreadEntry const& thread_entry)
{
  std::wostream& out = GetOutputStreamW();

  WriteNewline(out);
  WriteNamedHex(out, L"Usage", thread_entry.GetUsage(), 1);
  WriteNamedHex(out, L"ID", thread_entry.GetId(), 1);
  WriteNamedHex(out, L"Owner ID", thread_entry.GetOwnerId(), 1);
  WriteNamedHex(out, L"Base Priority", thread_entry.GetBasePriority(), 1);
  WriteNamedHex(out, L"Delta Priority", thread_entry.GetDeltaPriority(), 1);
  WriteNamedHex(out, L"Flags", thread_entry.GetFlags(), 1);
}

void DumpThreads(DWORD pid)
{
  std::wostream& out = GetOutputStreamW();

  WriteNewline(out);
  WriteNormal(out, L"Threads:", 0);

  hadesmem::ThreadList threads(pid);
  for (auto const& thread_entry : threads)
  {
    DumpThreadEntry(thread_entry);
  }
}

void DumpProcessEntry(hadesmem::ProcessEntry const& process_entry,
                      bool memonly = false)
{
  std::wostream& out = GetOutputStreamW();

  WriteNewline(out);
  WriteNamedHex(out, L"ID", process_entry.GetId(), 0);
  WriteNamedHex(out, L"Threads", process_entry.GetThreads(), 0);
  WriteNamedHex(out, L"Parent", process_entry.GetParentId(), 0);
  WriteNamedHex(out, L"Priority", process_entry.GetPriority(), 0);
  WriteNamedNormal(out, L"Name", process_entry.GetName(), 0);

  if (!memonly)
  {
    DumpThreads(process_entry.GetId());
  }

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

  if (!memonly)
  {
    // Using the Win32 API to get a processes path can fail for 'zombie'
    // processes. (QueryFullProcessImageName fails with ERROR_GEN_FAILURE.)
    // TODO: Implement this in terms of the native API (GetPathNative), but
    // translate the path to a Win32 path.
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

  // TODO: Put back the useful console output we used to get when we had this
  // implemented specifically for this tool.
  if (g_raw_base)
  {
    hadesmem::detail::DumpMemory(*process,
                                 g_flags,
                                 g_oep,
                                 nullptr,
                                 reinterpret_cast<void*>(g_raw_base),
                                 g_raw_size);
  }
  else
  {
    auto const module_base = [&]() -> void* {
      if (g_module_base)
      {
        return reinterpret_cast<void*>(g_module_base);
      }

      if (!g_module_name.empty())
      {
        hadesmem::Module const module{*process, g_module_name};
        return module.GetHandle();
      }

      return nullptr;
    }();

    hadesmem::detail::DumpMemory(*process, g_flags, g_oep, module_base);
  }
}

void DumpProcesses(bool memonly = false)
{
  std::wostream& out = GetOutputStreamW();

  WriteNewline(out);
  WriteNormal(out, L"Processes:", 0);

  hadesmem::ProcessList const processes;
  for (auto const& process_entry : processes)
  {
    DumpProcessEntry(process_entry, memonly);
  }
}
}

std::wstring GetCurrentFilePath()
{
  return g_current_file_path;
}

void SetCurrentFilePath(std::wstring const& path)
{
  g_current_file_path = path;
}

std::ostream& GetOutputStreamA()
{
  if (g_quiet)
  {
    thread_local static QuietStreamBuf<char> buf;
    thread_local static std::ostream str{&buf};
    return str;
  }
  else
  {
    return std::cout;
  }
}

std::wostream& GetOutputStreamW()
{
  if (g_quiet)
  {
    thread_local static QuietStreamBuf<wchar_t> buf;
    thread_local static std::wostream str{&buf};
    return str;
  }
  else
  {
    return std::wcout;
  }
}

void DumpPeFile(hadesmem::Process const& process,
                hadesmem::PeFile const& pe_file,
                std::wstring const& path)
{
  std::wostream& out = GetOutputStreamW();

  ClearWarnForCurrentFile();

  WriteNewline(out);
  std::wstring const architecture_str{pe_file.Is64() ? L"64-Bit File: Yes"
                                                     : L"64-Bit File: No"};
  WriteNormal(out, architecture_str, 1);

  std::uint32_t const k1MB = (1U << 20);
  std::uint32_t const k100MB = k1MB * 100;
  if (pe_file.GetSize() > k100MB)
  {
    // Not actually unsupported, just want to flag large files for use in perf
    // testing.
    WriteNewline(out);
    WriteNormal(out, L"WARNING! File is over 100MB.", 0);
    // WarnForCurrentFile(WarningType::kUnsupported);
  }

  DumpHeaders(process, pe_file);

  DumpSections(process, pe_file);

  DumpOverlay(process, pe_file);

  DumpTls(process, pe_file);

  DumpExports(process, pe_file);

  bool has_new_bound_imports_any = false;
  DumpImports(process, pe_file, has_new_bound_imports_any);

  DumpBoundImports(process, pe_file, has_new_bound_imports_any);

  DumpRelocations(process, pe_file);

  if (!g_quiet && g_strings)
  {
    DumpStrings(process, pe_file);
  }

  HandleWarnings(path);
}

void HandleLongOrUnprintableString(std::wstring const& name,
                                   std::wstring const& description,
                                   std::size_t tabs,
                                   WarningType warning_type,
                                   std::string value)
{
  std::wostream& out = GetOutputStreamW();

  // TODO: Fix perf for extremely long names. Instead of reading indefinitely
  // and then checking the size after the fact, we should perform a bounded
  // read.
  auto const unprintable = FindFirstUnprintableClassicLocale(value);
  std::size_t const kMaxNameLength = 1024;
  if (unprintable != std::string::npos)
  {
    WriteNormal(out,
                L"WARNING! Detected unprintable " + description +
                  L". Truncating.",
                tabs);
    WarnForCurrentFile(warning_type);
    value.erase(unprintable);
  }
  else if (value.size() > kMaxNameLength)
  {
    WriteNormal(out,
                L"WARNING! Detected suspiciously long " + description +
                  L". Truncating.",
                tabs);
    WarnForCurrentFile(warning_type);
    value.erase(kMaxNameLength);
  }
  WriteNamedNormal(out, name, value.c_str(), tabs);
}

// TODO: Use hadesmem::detail::TimestampToStringUtc instead.
bool ConvertTimeStamp(std::time_t time, std::wstring& str)
{
  std::tm local_time{};
  auto const local_time_conv = localtime_s(&local_time, &time);
  if (local_time_conv)
  {
    str = L"Invalid";
    return false;
  }

  char local_time_buf[26]{};
  auto const local_time_fmt = asctime_s(local_time_buf, &local_time);
  if (local_time_fmt)
  {
    str = L"Invalid";
    return false;
  }

  // MSDN documents the ctime class of functions as returning a string that is
  // exactly 26 characters long, of the form "Wed Jan 02 02:03:55 1980\n\0".
  // Don't copy the newline or the extra null terminator.
  str = hadesmem::detail::MultiByteToWideChar(
    std::string{local_time_buf, local_time_buf + 24});
  return true;
}

bool IsQuiet() noexcept
{
  return g_quiet;
}

int main(int argc, char* argv[])
{
  try
  {
    std::cout << "HadesMem Dumper [" << HADESMEM_VERSION_STRING << "]\n";

    TCLAP::CmdLine cmd("PE file format dumper", ' ', HADESMEM_VERSION_STRING);
    TCLAP::ValueArg<DWORD> pid_arg(
      "", "pid", "Target process id", false, 0, "DWORD");
    TCLAP::ValueArg<std::string> name_arg(
      "", "name", "Target process name", false, "", "string");
    TCLAP::MultiArg<std::string> path_arg(
      "", "path", "Target path (file or directory)", false, "string");
    TCLAP::SwitchArg all_arg("", "all", "No target, dump everything");
    std::vector<TCLAP::Arg*> xor_args{&pid_arg, &name_arg, &path_arg, &all_arg};
    cmd.xorAdd(xor_args);
    TCLAP::SwitchArg warned_arg(
      "", "warned", "Dump list of files which cause warnings", cmd);
    TCLAP::ValueArg<std::string> warned_file_arg(
      "",
      "warned-file",
      "Dump warned list to file instead of stdout",
      false,
      "",
      "string",
      cmd);
    TCLAP::SwitchArg warned_file_dynamic_arg(
      "",
      "warned-file-dynamic",
      "Dump warnings to file on the fly rather than at the end",
      cmd);
    TCLAP::SwitchArg quiet_arg(
      "", "quiet", "Only output status messages (no dumping)", cmd);
    TCLAP::SwitchArg memonly_arg("", "memonly", "Only do PE memory dumps", cmd);
    TCLAP::ValueArg<int> warned_type_arg("",
                                         "warned-type",
                                         "Filter warned file using warned type",
                                         false,
                                         -1,
                                         "int",
                                         cmd);
    TCLAP::ValueArg<std::size_t> threads_arg(
      "", "threads", "Number of threads", false, 0, "size_t", cmd);
    TCLAP::ValueArg<std::size_t> queue_factor_arg(
      "", "queue-factor", "Thread queue factor", false, 0, "size_t", cmd);
    TCLAP::SwitchArg strings_arg("", "strings", "Dump strings", cmd);
    TCLAP::SwitchArg use_disk_headers_arg(
      "",
      "use-disk-headers",
      "Use on-disk PE header for section layout when performing memory dumps",
      cmd);
    TCLAP::SwitchArg use_original_image_path_arg(
      "",
      "use-original-image-path",
      "Use original image path if it has been unmapped",
      cmd);
    TCLAP::SwitchArg reconstruct_imports_arg(
      "", "reconstruct-imports", "Reconstruct imports (build new ILT)", cmd);
    TCLAP::SwitchArg add_new_section_arg(
      "",
      "add-new-section",
      "Add new section to contain reconstructed imports (as opposed to being "
      "appended to the existing last section)",
      cmd);
    TCLAP::ValueArg<std::string> module_name_arg(
      "", "module-name", "Module to dump", false, "", "string", cmd);
    TCLAP::ValueArg<std::uintptr_t> module_base_arg(
      "", "module-base", "Module to dump", false, 0, "uintptr_t", cmd);
    TCLAP::ValueArg<std::uintptr_t> raw_base_arg(
      "", "raw-base", "Raw memory region base", false, 0, "uintptr_t", cmd);
    TCLAP::ValueArg<std::size_t> raw_size_arg(
      "", "raw-size", "Raw memory region size", false, 0, "size_t", cmd);
    TCLAP::ValueArg<DWORD> oep_arg(
      "",
      "oep",
      "Sets AddressOfEntryPoint for specified module",
      false,
      0,
      "DWORD",
      cmd);
    // TODO: Add --force-module-path to override the module path detection and
    // allow us to attempt to dump a PE file even if we don't have in-memory
    // headers or a file mapping backing the memory region.
    // TODO: Add --use-disk-iat to toggle using the disk IAT during import
    // reconstruction. Will be useful to allow using the disk headers for
    // section layout, but then ignoring it for imports.
    cmd.parse(argc, argv);

    g_quiet = quiet_arg.isSet();
    g_strings = strings_arg.isSet();
    g_flags |= use_disk_headers_arg.isSet()
                 ? hadesmem::detail::DumpFlags::kUseDiskHeaders
                 : 0;
    g_flags |= use_original_image_path_arg.isSet()
                 ? hadesmem::detail::DumpFlags::kUseOriginalImagePath
                 : 0;
    g_flags |= reconstruct_imports_arg.isSet()
                 ? hadesmem::detail::DumpFlags::kReconstructImports
                 : 0;
    g_flags |= add_new_section_arg.isSet()
                 ? hadesmem::detail::DumpFlags::kAddNewSection
                 : 0;
    g_oep = oep_arg.getValue();
    g_module_name =
      hadesmem::detail::MultiByteToWideChar(module_name_arg.getValue());
    g_module_base = module_base_arg.getValue();
    if (g_module_name.empty() && !g_module_base)
    {
      if (g_oep)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error()
          << hadesmem::ErrorString("OEP specified without module."));
      }
    }
    g_raw_base = raw_base_arg.getValue();
    g_raw_size = raw_size_arg.getValue();

    if (g_raw_base && !g_raw_size)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error() << hadesmem::ErrorString(
          "Please specify a size for raw region."));
    }

    if (g_raw_base && (g_module_base || !g_module_name.empty()))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error() << hadesmem::ErrorString(
          "Please specify either a raw region or a module."));
    }

    if (!g_module_name.empty() && g_module_base)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error() << hadesmem::ErrorString(
          "Please specify either a module name or a module base."));
    }

    SetWarningsEnabled(warned_arg.getValue());
    SetDynamicWarningsEnabled(warned_file_dynamic_arg.getValue());
    if (warned_file_arg.isSet())
    {
      SetWarnedFilePath(
        hadesmem::detail::MultiByteToWideChar(warned_file_arg.getValue()));
    }

    if (GetDynamicWarningsEnabled() && GetWarnedFilePath().empty())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error() << hadesmem::ErrorString(
          "Please specify a file path for dynamic warnings."));
    }

    int const warned_type = warned_type_arg.getValue();
    switch (warned_type)
    {
    case static_cast<int>(WarningType::kSuspicious):
      SetWarnedType(WarningType::kSuspicious);
      break;
    case static_cast<int>(WarningType::kUnsupported):
      SetWarnedType(WarningType::kUnsupported);
      break;
    case static_cast<int>(WarningType::kAll):
      SetWarnedType(WarningType::kAll);
      break;
    default:
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error() << hadesmem::ErrorString("Unknown warned type."));
      break;
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

    auto const threads = threads_arg.isSet() ? threads_arg.getValue() : 1;
    auto const queue_factor =
      queue_factor_arg.isSet() ? queue_factor_arg.getValue() : 1;
    hadesmem::detail::ThreadPool thread_pool{threads, queue_factor};

    if (pid_arg.isSet())
    {
      DWORD const pid = pid_arg.getValue();

      hadesmem::ProcessList const processes;
      auto iter =
        std::find_if(std::begin(processes),
                     std::end(processes),
                     [pid](hadesmem::ProcessEntry const& process_entry) {
                       return process_entry.GetId() == pid;
                     });
      if (iter != std::end(processes))
      {
        DumpProcessEntry(*iter, memonly_arg.isSet());
      }
      else
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error()
          << hadesmem::ErrorString("Failed to find requested process."));
      }
    }
    else if (name_arg.isSet())
    {
      auto const proc_name =
        hadesmem::detail::MultiByteToWideChar(name_arg.getValue());
      auto const proc_entry = hadesmem::GetProcessEntryByName(proc_name, false);
      DumpProcessEntry(proc_entry, memonly_arg.isSet());
    }
    else if (path_arg.isSet())
    {
      // TODO: Use backup semantics flags and try to get backup privilege in
      // order to make directory enumeration find more files.
      auto const path_args = path_arg.getValue();
      for (auto const& path : path_args)
      {
        auto const path_wide = hadesmem::detail::MultiByteToWideChar(path);
        if (hadesmem::detail::IsDirectory(path_wide))
        {
          DumpDir(path_wide, thread_pool);
        }
        else
        {
          DumpFile(path_wide);
        }
      }
    }
    else
    {
      DumpThreads(static_cast<DWORD>(-1));

      DumpProcesses(memonly_arg.isSet());

      std::wcout << "\nFiles:\n";

      // TODO: Enumerate all volumes.
      std::wstring const self_path = hadesmem::detail::GetSelfPath();
      std::wstring const root_path = hadesmem::detail::GetRootPath(self_path);
      DumpDir(root_path, thread_pool);
    }

    thread_pool.WaitForEmpty();

    if (GetWarningsEnabled())
    {
      if (!GetWarnedFilePath().empty() && !GetDynamicWarningsEnabled())
      {
        std::unique_ptr<std::wfstream> warned_file_ptr(
          hadesmem::detail::OpenFile<wchar_t>(GetWarnedFilePath(),
                                              std::ios::out));
        std::wfstream& warned_file = *warned_file_ptr;
        if (!warned_file)
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(
            hadesmem::Error()
            << hadesmem::ErrorString("Failed to open warned file for output."));
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

    return 1;
  }
}
