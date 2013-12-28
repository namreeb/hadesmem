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
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/smart_handle.hpp>
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
// image or inside the headers, non-standard alignments, etc.

// TODO: Split this tool up into multiple source files (more).

namespace
{

// Record all modules (on disk) which cause a warning when dumped, to make it
// easier to isolate files which require further investigation.
// TODO: Clean this up.
// TODO: Support logging the warned file list 'on the fly' rather than at the
// end if the warning list is being output to disk. This will make it more
// useful when doing extremely large runs.
bool g_warned = false;
bool g_warned_enabled = false;
std::vector<std::wstring> g_all_warned;

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

std::wstring MakeExtendedPath(std::wstring const& path)
{
  return path.size() >= 4 && path.substr(0, 4) == L"\\\\?\\"
           ? path
           : L"\\\\?\\" + path;
}

void DumpRegions(hadesmem::Process const& process)
{
  std::wcout << "\nRegions:\n";

  hadesmem::RegionList const regions(process);
  for (auto const& region : regions)
  {
    std::wcout << "\n";
    std::wcout << "\tBase: " << PtrToString(region.GetBase()) << "\n";
    std::wcout << "\tAllocation Base: " << PtrToString(region.GetAllocBase())
               << "\n";
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

void DumpPeFile(hadesmem::Process const& process,
                hadesmem::PeFile const& pe_file,
                std::wstring const& path)
{
  DumpHeaders(process, pe_file);

  DumpSections(process, pe_file);

  DumpTls(process, pe_file);

  DumpExports(process, pe_file);

  DumpImports(process, pe_file);

  if (g_warned)
  {
    g_all_warned.push_back(path);
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

void DumpFile(std::wstring const& path)
{
  g_warned = false;

#if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  std::ifstream file(path, std::ios::binary | std::ios::ate);
#else  // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  // libstdc++ doesn't support wide character overloads for ifstream's
  // construtor. :(
  std::ifstream file(hadesmem::detail::WideCharToMultiByte(path),
                     std::ios::binary | std::ios::ate);
#endif // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  if (!file)
  {
    std::wcout << "\nFailed to open file.\n";
    return;
  }

  std::streampos const size = file.tellg();
  if (size <= 0)
  {
    std::wcout << "\nEmpty or invalid file.\n";
    return;
  }

  if (!file.seekg(0, std::ios::beg))
  {
    std::wcout << "\nWARNING! Seeking to beginning of file failed (1).\n";
    return;
  }

  // Peek for the MZ header before reading the whole file.
  std::vector<char> mz_buf(2);
  if (!file.read(mz_buf.data(), 2))
  {
    std::wcout << "\nWARNING! Failed to read header signature.\n";
    return;
  }

  // Check for MZ signature
  if (mz_buf[0] != 'M' && mz_buf[1] != 'Z')
  {
    std::wcout << "\nNot a PE file (Pass 1).\n";
    return;
  }

  if (!file.seekg(0, std::ios::beg))
  {
    std::wcout << "\nWARNING! Seeking to beginning of file failed (2).\n";
    return;
  }

  std::vector<char> buf(static_cast<std::size_t>(size));

  if (!file.read(buf.data(), static_cast<std::streamsize>(size)))
  {
    std::wcout << "\nWARNING! Failed to read file data.\n";
    return;
  }

  hadesmem::Process const process(GetCurrentProcessId());

  hadesmem::PeFile const pe_file(process,
                                 buf.data(),
                                 hadesmem::PeFileType::Data,
                                 static_cast<DWORD>(buf.size()));

  try
  {
    hadesmem::NtHeaders const nt_hdr(process, pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\nNot a PE file or wrong architecture (Pass 2).\n";
    return;
  }

  DumpPeFile(process, pe_file, path);
}

void DumpDir(std::wstring const& path)
{
  std::wcout << "\nEntering dir: \"" << path << "\".\n";

  std::wstring path_real(path);
  if (path_real.back() == L'\\')
  {
    path_real.pop_back();
  }

  WIN32_FIND_DATA find_data;
  ZeroMemory(&find_data, sizeof(find_data));
  hadesmem::detail::SmartFindHandle const handle(
    ::FindFirstFile((path_real + L"\\*").c_str(), &find_data));
  if (!handle.IsValid())
  {
    DWORD const last_error = ::GetLastError();
    if (last_error == ERROR_FILE_NOT_FOUND)
    {
      std::wcout << "\nDirectory is empty.\n";
      return;
    }
    if (last_error == ERROR_ACCESS_DENIED)
    {
      std::wcout << "\nAccess denied to directory.\n";
      return;
    }
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error() << hadesmem::ErrorString("FindFirstFile failed.")
                        << hadesmem::ErrorCodeWinLast(last_error));
  }

  do
  {
    std::wstring const cur_file = find_data.cFileName;
    if (cur_file == L"." || cur_file == L"..")
    {
      continue;
    }

    std::wstring const cur_path =
      MakeExtendedPath(path_real + L"\\" + cur_file);

    std::wcout << "\nCurrent path: \"" << cur_path << "\".\n";

    try
    {
      if (hadesmem::detail::IsDirectory(cur_path) &&
          !hadesmem::detail::IsSymlink(cur_path))
      {
        DumpDir(cur_path);
      }

      DumpFile(cur_path);
    }
    catch (hadesmem::Error const& e)
    {
      auto const last_error_ptr =
        boost::get_error_info<hadesmem::ErrorCodeWinLast>(e);
      if (last_error_ptr && *last_error_ptr == ERROR_SHARING_VIOLATION)
      {
        std::wcout << "\nSharing violation.\n";
        continue;
      }

      if (last_error_ptr && *last_error_ptr == ERROR_ACCESS_DENIED)
      {
        std::wcout << "\nAccess denied.\n";
        continue;
      }

      if (last_error_ptr && *last_error_ptr == ERROR_FILE_NOT_FOUND)
      {
        std::wcout << "\nFile not found.\n";
        continue;
      }

      throw;
    }
  } while (::FindNextFile(handle.GetHandle(), &find_data));

  DWORD const last_error = ::GetLastError();
  if (last_error == ERROR_NO_MORE_FILES)
  {
    return;
  }
  HADESMEM_DETAIL_THROW_EXCEPTION(
    hadesmem::Error() << hadesmem::ErrorString("FindNextFile failed.")
                      << hadesmem::ErrorCodeWinLast(last_error));
}
}

void WarnForCurrentFile()
{
  g_warned = true;
}

std::wstring PtrToString(void const* const ptr)
{
  std::wostringstream str;
  str.imbue(std::locale::classic());
  str << std::hex << reinterpret_cast<DWORD_PTR>(ptr);
  return str.str();
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
      "dump warned list to file instead of stdout");

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
      if (var_map.count("warned-file"))
      {
        std::wstring const warned_file_path =
          var_map["warned-file"].as<std::wstring>();
#if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
        std::wofstream warned_file(warned_file_path);
#else  // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
        // libstdc++ doesn't support wide character overloads for ifstream's
        // construtor. :(
        std::wofstream warned_file(
          hadesmem::detail::WideCharToMultiByte(warned_file_path));
#endif // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
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
    std::cerr << "\nError!\n";
    std::cerr << boost::current_exception_diagnostic_information() << '\n';

    return 1;
  }
}
