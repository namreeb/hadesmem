// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <tuple>

#include <windows.h>
#include <psapi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/peb.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/pelib/dos_header.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/pelib/export_dir.hpp>
#include <hadesmem/pelib/export_list.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/import_dir_list.hpp>
#include <hadesmem/pelib/import_thunk.hpp>
#include <hadesmem/pelib/import_thunk_list.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/section.hpp>
#include <hadesmem/pelib/section_list.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_helpers.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/region_list.hpp>

// TODO: Clean this up. It's a mess right now...

// TODO: Harden PE format parsing.

// TODO: Investigate spurious failures. (During export mapping?)

namespace hadesmem
{
namespace detail
{
// TODO: Type safety.
struct DumpFlags
{
  enum : std::uint32_t
  {
    kNone = 0,
    kUseDiskHeaders = 1 << 0,
    kUseOriginalImagePath = 1 << 1,
    kReconstructImports = 1 << 2,
    kAddNewSection = 1 << 3,
    kInvalidFlagMaxValue = 1 << 4,
  };
};

inline std::uint64_t RoundUp(std::uint64_t n, std::uint64_t m)
{
  if (!m)
  {
    return n;
  }

  auto const r = n % m;
  if (!r)
  {
    return n;
  }

  return n + m - r;
}

inline void WriteDumpFile(Process const& process,
                          std::wstring const& region_name,
                          void const* buffer,
                          std::size_t size,
                          std::wstring const& dir_name = L"dumps")
{
  HADESMEM_DETAIL_TRACE_A("Creating dump dir.");

  auto const dumps_dir = CombinePath(GetSelfDirPath(), dir_name);
  CreateDirectoryWrapper(dumps_dir, false);

  HADESMEM_DETAIL_TRACE_A("Generating file name.");

  auto const proc_path = GetPath(process);
  auto const proc_name_dir =
    CombinePath(dumps_dir, proc_path.substr(proc_path.rfind(L'\\') + 1));
  CreateDirectoryWrapper(proc_name_dir, false);
  auto const proc_pid_dir =
    CombinePath(proc_name_dir, std::to_wstring(process.GetId()));
  CreateDirectoryWrapper(proc_pid_dir, false);
  std::wstring dump_path = CombinePath(proc_pid_dir, region_name);

  if (DoesFileExist(dump_path))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("Target file already exists."));
  }

  HADESMEM_DETAIL_TRACE_A("Opening file.");

  auto const dump_file =
    OpenFile<char>(dump_path, std::ios::out | std::ios::binary);
  if (!*dump_file)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("Unable to open dump file."));
  }

  HADESMEM_DETAIL_TRACE_A("Writing file.");

  if (!dump_file->write(static_cast<char const*>(buffer), size))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("Unable to write to dump file."));
  }
}

inline std::wstring GetRegionPathOrDefault(Process const& process,
                                           void* p,
                                           void* imagebase,
                                           bool use_original_image_path)
{
  std::vector<wchar_t> mapped_file_name(HADESMEM_DETAIL_MAX_PATH_UNICODE);
  if (::GetMappedFileNameW(process.GetHandle(),
                           p,
                           mapped_file_name.data(),
                           static_cast<DWORD>(mapped_file_name.size())))
  {
    return mapped_file_name.data();
  }

  DWORD const last_error = ::GetLastError();
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "WARNING! GetMappedFileNameW failed. LastError: [%08lX].", last_error);

  // TODO: Instead of a config flag, is there a better solution to this problem
  // that doesn't involve guessing?
  if (use_original_image_path && p == imagebase)
  {
    try
    {
      return hadesmem::GetPathNative(process);
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A("WARNING! Failed to get default path.");
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
    }
  }

  return {};
}

// TODO: Support files mapped from UNC shares? Any other corner cases?
inline std::wstring GetRegionPath(Process const& process,
                                  void* p,
                                  void* imagebase,
                                  bool use_original_image_path)
{
  try
  {
    auto const mapped_file_name =
      GetRegionPathOrDefault(process, p, imagebase, use_original_image_path);
    if (!mapped_file_name.size())
    {
      return {};
    }

    std::vector<wchar_t> drive_strings(HADESMEM_DETAIL_MAX_PATH_UNICODE);
    if (!GetLogicalDriveStringsW(static_cast<DWORD>(drive_strings.size() - 1),
                                 drive_strings.data()))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "WARNING! GetLogicalDriveStringsW failed. LastError: [%08lX].",
        last_error);
      return {};
    }

    TCHAR* d = drive_strings.data();

    // TODO: Replace this with NtQuerySymbolicLinkObject so we can correctly
    // handle network paths and other tricky cases?
    do
    {
      std::array<wchar_t, 3> drive{*d, ':', '\0'};
      std::vector<wchar_t> device_path(HADESMEM_DETAIL_MAX_PATH_UNICODE);
      if (::QueryDosDeviceW(drive.data(),
                            device_path.data(),
                            static_cast<DWORD>(device_path.size())))
      {
        auto const device_path_len =
          static_cast<DWORD>(std::wstring(device_path.data()).size());
        if (device_path_len < MAX_PATH)
        {
          if (_wcsnicmp(mapped_file_name.data(),
                        device_path.data(),
                        device_path_len) == 0 &&
              mapped_file_name[device_path_len] == L'\\')
          {
            return std::wstring(drive.data()) +
                   (mapped_file_name.data() + device_path_len);
          }
        }
      }

      while (*d++)
      {
        // Advance to next string.
      }
    } while (*d);

    return {};
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("WARNING! Error retriving region path.");
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    return {};
  }
}

inline std::wstring MakeNameFromPath(std::wstring const& path)
{
  // Don't need to worry about name overlap because when we write the file to
  // disk we prepend the base address to the filename.
  return path.empty() ? L"unknown_hadesmem.dll"
                      : path.substr(path.rfind(L'\\') + 1);
}

inline std::int32_t GetModulePriority(std::wstring const& name,
                                      std::wstring const& path)
{
  if (path.empty())
  {
    return -1;
  }

  auto const name_upper = ToUpperOrdinal(name);
  if (name_upper == L"KERNEL32.DLL")
  {
    return 3;
  }
  else if (name_upper == L"OLE32.DLL")
  {
    return 2;
  }
  else if (name_upper == L"NTDLL.DLL" || name_upper == L"SHLWAPI.DLL" ||
           name_upper == L"SHIMENG.DLL" ||
           !_wcsnicmp(name_upper.c_str(), L"API-", 4) ||
           !_wcsnicmp(name_upper.c_str(), L"EXT-", 4))
  {
    return 0;
  }
  else if (name_upper == L"KERNELBASE.DLL")
  {
    return -1;
  }
  else
  {
    return 1;
  }
}

struct ModuleLight
{
  ModuleLight(Process const& process,
              void* base,
              std::size_t size,
              bool use_disk_headers,
              void* imagebase,
              bool use_original_image_path)
    : process_{process},
      pe_file_{process_, base, PeFileType::kImage, static_cast<DWORD>(size)},
      path_{GetRegionPath(process, base, imagebase, use_original_image_path)},
      name_{MakeNameFromPath(path_)},
      priority_{GetModulePriority(name_, path_)}
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(
      L"Base: [%p]. Name: [%s]. Priority: [%d]. Path: [%s].",
      base,
      name_.c_str(),
      priority_,
      path_.c_str());

    if (path_.empty())
    {
      use_disk_headers = false;
    }

    // Try on-disk validation first in case the in-memory representation is
    // corrupted.
    if (use_disk_headers && !path_.empty())
    {
      use_disk_headers = ValidateDiskHeaders();
    }

    if (!use_disk_headers)
    {
      // If on-disk headers are not found or not requested, try to verify using
      // in-memory headers.
      NtHeaders nt_headers{process_, pe_file_};

      HADESMEM_DETAIL_TRACE_A("Successfully verified NT headers.");
    }
  }

  bool ValidateDiskHeaders()
  {
    try
    {
      auto buffer = PeFileToBuffer(path_);
      Process local_process{::GetCurrentProcessId()};
      PeFile pe_file{local_process,
                     buffer.data(),
                     PeFileType::kData,
                     static_cast<DWORD>(buffer.size())};
      NtHeaders nt_headers{local_process, pe_file};
      return true;
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(
        L"WARNING! Failed to verify on-disk headers. Path: [%s].",
        path_.c_str());
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      return false;
    }
  }

  Process process_;
  PeFile pe_file_;
  std::wstring path_;
  std::wstring name_;
  std::int32_t priority_;
};

struct ExportLight
{
  ModuleLight const* module_;

  bool by_name_;
  std::string name_;
  DWORD ordinal_;
};

struct ProcessLight
{
  ProcessLight() = default;
  ProcessLight(ProcessLight const&) = delete;
  ProcessLight& operator=(ProcessLight const&) = delete;
  ProcessLight(ProcessLight&&) = default;
  ProcessLight& operator=(ProcessLight&&) = default;
  ~ProcessLight() = default;

  std::vector<ModuleLight> modules_;
  std::map<void*, std::vector<ExportLight>> export_map_;
};

struct PeDumper
{
public:
  PeDumper(Process const& process,
           void* base,
           std::uint32_t flags,
           DWORD oep,
           ModuleLight const* m = nullptr)
    : process_{&process},
      base_{base},
      flags_{flags},
      oep_{oep},
      m_{m},
      process_light_(MakeProcessLight())
  {
  }

  void Dump()
  {
    if (base_)
    {
      DumpSingleModule(base_);
    }
    else
    {
      DumpAllModules();
    }
  }

  void Reset()
  {
    process_light_ = MakeProcessLight();
  }

private:
  ProcessLight BuildModuleList()
  {
    HADESMEM_DETAIL_TRACE_A("Building module list.");

    HADESMEM_DETAIL_TRACE_A("Getting page size.");

    SYSTEM_INFO sys_info{};
    ::GetSystemInfo(&sys_info);
    auto const page_size = sys_info.dwPageSize;

    HADESMEM_DETAIL_TRACE_A("Getting PEB.");

    auto const peb = GetPeb(*process_);

    HADESMEM_DETAIL_TRACE_FORMAT_A("ImageBase: [%p].", peb.ImageBaseAddress);

    RegionList regions(*process_);
    ProcessLight process_info;
    for (auto const& region : regions)
    {
      auto const region_beg = static_cast<std::uint8_t*>(region.GetBase());
      auto const region_end = region_beg + region.GetSize();
      if (!region_beg || region_end < region_beg)
      {
        continue;
      }

      if (!CanRead(*process_, region_beg) ||
          IsBadProtect(*process_, region_beg))
      {
        continue;
      }

      // TODO: Add a config flag to search for modules which are not aligned on
      // a page boundary.
      for (auto p = region_beg; p + page_size <= region_end; p += page_size)
      {
        HandleModuleCandidate(p, process_info, peb);
      }
    }

    return process_info;
  }

  // TODO: Support dumping and rebuilding modules which have had their headers
  // erased.
  void HandleModuleCandidate(std::uint8_t* p,
                             ProcessLight& process_info,
                             winternl::PEB const& peb)
  {
    auto const mz = Read<std::array<char, 2>>(*process_, p);
    if (mz[0] != 'M' || mz[1] != 'Z')
    {
      return;
    }

    HADESMEM_DETAIL_TRACE_FORMAT_A("Found module candidate. Base: [%p].", p);

    // We potentially over-estimate module size here, but it's probably best
    // to over-estimate in the case where we have no headers, and in the
    // case where we do have headers it's not a big deal because we only use
    // this size to determine the maximum amount of data to read, not the
    // minimum.
    auto const size = GetModuleRegionSize(*process_, p, true);
    if (size > static_cast<DWORD>(-1))
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Region too large to be a module. Size: [%IX].", size);
      return;
    }

    HADESMEM_DETAIL_TRACE_FORMAT_A("Calculated module size: [%08lX].",
                                   static_cast<DWORD>(size));

    try
    {
      HADESMEM_DETAIL_TRACE_A("Checking for valid headers, and retrieving "
                              "region name and optional path.");
      process_info.modules_.emplace_back(
        *process_,
        p,
        size,
        !!(flags_ & DumpFlags::kUseDiskHeaders),
        peb.ImageBaseAddress,
        !!(flags_ & DumpFlags::kUseOriginalImagePath));
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A("WARNING! Invalid headers. Base: [%p].",
                                     p);
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
    }
  }

  void BuildExportMap(ProcessLight& process_info)
  {
    if (!(flags_ & DumpFlags::kReconstructImports))
    {
      HADESMEM_DETAIL_TRACE_A("Skipping export map generation because import "
                              "reconstruction is disabled.");
      return;
    }

    HADESMEM_DETAIL_TRACE_A("Building export map.");

    for (auto const& m : process_info.modules_)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Module: [%s].", m.name_.c_str());

      // TODO: Use on disk headers here if we have the option? Or both?
      // TODO: If the module is loaded in our own process we could theoretically
      // enumerate exports locally?
      ExportList exports(m.process_, m.pe_file_);
      for (auto const& e : exports)
      {
        ModuleLight const* resolved_module = &m;

        auto va = e.GetVa();
        if (e.IsForwarded())
        {
          HADESMEM_DETAIL_TRACE_FORMAT_A(
            "Got forwarded export. Name: [%s]. Ordinal: [%d]. Forwarder: [%s].",
            e.GetName().c_str(),
            e.GetProcedureNumber(),
            e.GetForwarder().c_str());

          try
          {
            // We don't need special handling for API sets here because we are
            // always resolving everything in the context of the remote process,
            // and Windows should've already done all the redirections for us.
            // One thing we could do is use the API Set Schema information to
            // "undo" the redirections (i.e. generate an import to
            // Kernel32.AddDllDirectory instead of KernelBase.AddDllDirectory ,
            // currently we get the 'wrong' one due to redirection by
            // api-ms-win-core-libraryloader-l1-1-0), but that seems to be
            // relatively unimportant as it doesn't appear to affect the
            // behavior of the file.
            // TODO: This is incorrect. For example, we're currently resolving
            // ole32.dll!CLSIDFromProgID as combase.dll!CLSIDFromProgID. We
            // can't establish the link to prefer ole32.dll because we don't
            // resolve the API set. This needs to be fixed in order for the
            // import filtering to work effectively!

            // TODO: However, not performing the aforementioned transformation
            // might weaken some of the algorithms we use or intend to use. For
            // example, if we have 5 imports in a row from Kernel32, then one
            // from KernelBase, then another 5 from Kernel32 (or worse, a bunch
            // of contiguous thunks from Kenrel32 which have all been redirected
            // to different modules), we might mistake the single thunk(s) as
            // invalid, because right now we get a lot of real invalid thunks so
            // we need to perform some filtering, but that would break this case
            // unless we detect and handle it.

            // TODO: Improve support for modules which import directly from the
            // API set modules. They break the link between the high level and
            // low level modules. E.g. Kernel32.AddDllDirectory ->
            // api-ms-win-core-libraryloader-l1-1-0.AddDllDirectory ->
            // KernelBase.AddDllDirectory. If we can't resolve the API set
            // module then we will always select KernelBase for that API even if
            // Kernel32 would be a better fit (e.g. for import coalescing).

            // TODO: Detect and handle cases where imports have been shimmed?

            va = GetProcAddressFromExport(*process_, e);

            auto forwarder_module_name = e.GetForwarderModule();
            forwarder_module_name =
              forwarder_module_name.find('.') != std::string::npos
                ? forwarder_module_name
                : (forwarder_module_name + ".DLL");

            auto const iter = std::find_if(
              std::begin(process_info.modules_),
              std::end(process_info.modules_),
              [&](ModuleLight const& i) {
                return ToUpperOrdinal(i.name_) ==
                       ToUpperOrdinal(hadesmem::detail::MultiByteToWideChar(
                         forwarder_module_name));
              });
            if (iter != std::end(process_info.modules_))
            {
              HADESMEM_DETAIL_TRACE_FORMAT_A(
                "Resolved forwarded module. Name: [%s].",
                forwarder_module_name.c_str());

              // TODO: Fix this. We need to make sure we look up the correct new
              // procedure number etc to create the ExportLight struct, we can't
              // just swap out the module.
              // resolved_module = &*iter;
            }

            HADESMEM_DETAIL_TRACE_FORMAT_A(
              "Resolved forwarded export. VA: [%p].", va);
          }
          catch (...)
          {
            auto const name_upper = m.name_;

            // Quiet debug spew for some expected/intentional failures.
            if (hadesmem::detail::ToUpperOrdinal(m.name_) == L"SHUNIMPL.DLL")
            {
              HADESMEM_DETAIL_TRACE_FORMAT_W(L"Ignoring expected failure "
                                             L"resolving forwarded export. "
                                             L"Module: [%s]. Name: [%hs].",
                                             m.name_.c_str(),
                                             e.GetName().c_str());
              continue;
            }

            HADESMEM_DETAIL_TRACE_A(
              "WARNING! Failed to resolve forwarded export.");
            HADESMEM_DETAIL_TRACE_FORMAT_W(
              L"Module: [%s]. Name: [%hs]. Ordinal: [%d]. Forwarder: [%hs].",
              m.name_.c_str(),
              e.GetName().c_str(),
              e.GetProcedureNumber(),
              e.GetForwarder().c_str());
            HADESMEM_DETAIL_TRACE_A(
              boost::current_exception_diagnostic_information().c_str());
            continue;
          }
        }
        else
        {
          HADESMEM_DETAIL_TRACE_FORMAT_A(
            "Got export. VA: [%p]. Name: [%s]. Ordinal: [%d].",
            va,
            e.GetName().c_str(),
            e.GetProcedureNumber());
        }

        if (!va)
        {
          HADESMEM_DETAIL_TRACE_A("WARNING! Skipping zero VA.");
          continue;
        }

        if (e.IsVirtualVa())
        {
          HADESMEM_DETAIL_TRACE_FORMAT_A(
            "WARNING! Skipping virtual VA. VA: [%p].", va);
          continue;
        }

        HADESMEM_DETAIL_TRACE_A("Adding to export map.");

        process_info.export_map_[va].emplace_back(ExportLight{
          resolved_module, e.ByName(), e.GetName(), e.GetProcedureNumber()});
      }
    }

    HADESMEM_DETAIL_TRACE_FORMAT_A("Num Modules: [%Iu].",
                                   process_info.modules_.size());
    HADESMEM_DETAIL_TRACE_FORMAT_A("Num Export VAs: [%Iu].",
                                   process_info.export_map_.size());
  }

  void SortExportMapModules(ProcessLight& process_info)
  {
    HADESMEM_DETAIL_TRACE_A("Sorting export map modules by priority.");

    for (auto& i : process_info.export_map_)
    {
      auto& modules = i.second;
      std::sort(std::begin(modules),
                std::end(modules),
                [](ExportLight const& lhs, ExportLight const& rhs) {
                  return lhs.module_->priority_ < rhs.module_->priority_;
                });
    }
  }

  ProcessLight MakeProcessLight()
  {
    auto process_info = BuildModuleList();
    BuildExportMap(process_info);
    SortExportMapModules(process_info);
    return process_info;
  }

  struct SectionData
  {
    DWORD raw_data_;
    DWORD raw_size_;
    DWORD virtual_size_;
  };

  std::vector<SectionData> CopySectionData(Process const& local_process,
                                           PeFile const& pe_file_headers,
                                           NtHeaders const& nt_headers,
                                           std::vector<std::uint8_t>& raw_new,
                                           std::vector<std::uint8_t> const& raw)
  {
    HADESMEM_DETAIL_TRACE_A("Copying section data.");

    std::vector<SectionData> section_datas;

    SectionList const sections(local_process, pe_file_headers);
    for (auto const& section : sections)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "VirtualAddress: [%08lX]. PointerToRawData: [%08lX].",
        section.GetVirtualAddress(),
        section.GetPointerToRawData());

      // TODO: Do some validation here based on memory layout?
      // TODO: Ensure all the crazy scenarios like overlapping virtual sections,
      // bogus header sizes and pointers, etc. all work.
      // TODO: When rounding up from raw to virtual, we should check for
      // zero-fill at the end and then drop as much data as possible in order to
      // reduce file size.
      // TODO: Validate section virtual sizes against the VA region sizes?
      // TODO: Automatically align sections? (e.g. Bump section virtual size
      // from 0x400 to 0x1000).
      // TODO: It's not always correct to use VirtualSize here. It's great for
      // analysis reasons, but if you want runnable dumps some programs might
      // rely on that virtual space to be zero'd out at runtime.
      auto const section_size = static_cast<DWORD>(RoundUp(
        (std::max)(section.GetVirtualSize(), section.GetSizeOfRawData()),
        nt_headers.GetSectionAlignment()));
      auto const ptr_raw_data_new_tmp =
        section.GetPointerToRawData() < raw_new.size()
          ? static_cast<DWORD>(
              RoundUp(raw_new.size(), nt_headers.GetFileAlignment()))
          : section.GetPointerToRawData();
      auto const ptr_raw_data_new = static_cast<DWORD>(
        RoundUp(ptr_raw_data_new_tmp, nt_headers.GetFileAlignment()));

      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "New PointerToRawData: [%08lX]. New SizeOfRawData: [%08lX].",
        ptr_raw_data_new,
        section_size);

      if (ptr_raw_data_new > raw_new.size())
      {
        raw_new.resize(ptr_raw_data_new);
      }

      section_datas.emplace_back(
        SectionData{ptr_raw_data_new, section_size, section_size});

      if (section.GetVirtualAddress() >= raw.size())
      {
        HADESMEM_DETAIL_TRACE_A("WARNING! Not writing any data for current "
                                "section due to out-of-bounds VA.");
        continue;
      }

      auto const raw_data = raw.data() + section.GetVirtualAddress();
      auto const raw_data_end =
        (std::min)(raw_data + section_size, raw.data() + raw.size());
      if (raw_data_end != raw_data + section_size)
      {
        HADESMEM_DETAIL_TRACE_A("WARNING! Truncating read for current section "
                                "due to out-of-bounds VA.");
      }

      auto const new_size = raw_new.size() + section_size;
      raw_new.reserve(new_size);
      std::copy(raw_data, raw_data_end, std::back_inserter(raw_new));
      raw_new.resize(new_size);

      auto raw_section_size = section_size;
      for (auto p = raw_new.data() + raw_new.size() - sizeof(std::uint32_t);
           (p >
            raw_new.data() + ptr_raw_data_new + section.GetSizeOfRawData()) &&
           *reinterpret_cast<std::uint32_t*>(p);
           p -= sizeof(std::uint32_t))
      {
        raw_section_size -= sizeof(std::uint32_t);
      }

      raw_section_size = static_cast<DWORD>(
        RoundUp(raw_section_size, nt_headers.GetFileAlignment()));

      section_datas.back().raw_size_ = raw_section_size;
    }

    return section_datas;
  }

  void FixSectionHeaders(SectionList& sections_new,
                         std::vector<SectionData> const& section_datas)
  {
    HADESMEM_DETAIL_TRACE_A("Fixing section headers.");

    std::size_t n = 0;
    for (auto& section : sections_new)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A("PointerToRawData: [%08lX] -> [%08lX]. "
                                     "SizeOfRawData: [%08lX] -> [%08lX]. "
                                     "VirtualSize: [%08lX] -> [%08lX].",
                                     section.GetPointerToRawData(),
                                     section_datas[n].raw_data_,
                                     section.GetSizeOfRawData(),
                                     section_datas[n].raw_size_,
                                     section.GetVirtualSize(),
                                     section_datas[n].virtual_size_);

      section.SetPointerToRawData(section_datas[n].raw_data_);
      section.SetSizeOfRawData(section_datas[n].raw_size_);
      section.SetVirtualSize(section_datas[n].virtual_size_);
      section.UpdateWrite();
      ++n;
    }
  }

  NtHeaders FixNtHeaders(Process const& local_process,
                         PeFile const& pe_file_new,
                         void* base,
                         bool has_disk_headers,
                         std::unique_ptr<PeFile> const& pe_file_disk)
  {
    HADESMEM_DETAIL_TRACE_A("Fixing NT headers.");

    NtHeaders nt_headers_new(local_process, pe_file_new);

    // TODO: Add flag.
    auto const image_base_new =
      static_cast<ULONGLONG>(reinterpret_cast<ULONG_PTR>(base));
    HADESMEM_DETAIL_TRACE_FORMAT_A("ImageBase: [%016llX] -> [%016llX].",
                                   nt_headers_new.GetImageBase(),
                                   image_base_new);
    nt_headers_new.SetImageBase(image_base_new);

    if (oep_)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A("AddressOfEntryPoint: [%08lX] -> [%08lX].",
                                     nt_headers_new.GetAddressOfEntryPoint(),
                                     oep_);
      nt_headers_new.SetAddressOfEntryPoint(oep_);
    }

    // TODO: Attempt this even in the case that the use hasn't requested use of
    // disk headers, because something is better than nothing. Just make sure if
    // we can't load the file that we fail gracefully and continue.
    else if (has_disk_headers && !nt_headers_new.GetAddressOfEntryPoint())
    {
      NtHeaders nt_headers_disk(local_process, *pe_file_disk);
      HADESMEM_DETAIL_TRACE_FORMAT_A("AddressOfEntryPoint: [%08lX] -> [%08lX].",
                                     nt_headers_new.GetAddressOfEntryPoint(),
                                     nt_headers_disk.GetAddressOfEntryPoint());
      nt_headers_new.SetAddressOfEntryPoint(
        nt_headers_disk.GetAddressOfEntryPoint());
    }

    nt_headers_new.UpdateWrite();

    return nt_headers_new;
  }

  std::map<DWORD, ExportLight const*>
    DoMemoryScan(std::vector<std::uint8_t>& raw_new,
                 std::map<void*, std::vector<ExportLight>> const& export_map,
                 void* base,
                 std::size_t pe_size,
                 Process const& local_process,
                 PeFile const& pe_file_new)
  {
    HADESMEM_DETAIL_TRACE_A("Performing memory scan.");

    // TODO: Support other scanning algorithms.
    // TODO: Be smarter about deciding which imports are legitimate and which
    // are false positives.
    // TODO: Ensure imports to hidden modules are found correctly. Required
    // for when a manually mapped module is manually mapping its dependencies.
    // TODO: Check section characteristics as an additional heuristic?
    std::map<DWORD, ExportLight const*> fixup_map;
    bool fixup_adjacent = false;
    for (auto p = raw_new.data(); p < raw_new.data() + raw_new.size() - 3;
         p += 4)
    {
      auto const offset =
        static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(p) -
                           reinterpret_cast<std::uintptr_t>(raw_new.data()));

      auto va = *reinterpret_cast<void**>(p);
      auto i = export_map.find(va);
      if (i == std::end(export_map))
      {
        // TODO: Make sure this doesn't overlap with any previous fixups,
        // redirected or otherwise?
        auto const resolved_va = ResolveRedirectedImport(va);
        if (!resolved_va)
        {
          fixup_adjacent = false;
          continue;
        }

        HADESMEM_DETAIL_TRACE_FORMAT_A(
          "Resolved redirected import (unverified). Old: [%p]. New: [%p].",
          va,
          resolved_va);

        i = export_map.find(resolved_va);
        if (i == std::end(export_map))
        {
          HADESMEM_DETAIL_TRACE_A("WARNING! Successfully resolved redirected "
                                  "import, but then failed to match it to an "
                                  "export.");
          fixup_adjacent = false;
          continue;
        }
      }

      if (va >= base && va <= static_cast<std::uint8_t*>(base) + pe_size)
      {
        HADESMEM_DETAIL_TRACE_FORMAT_A(
          "WARNING! Skipping VA in current module. Offset: [%08X]. VA: [%p].",
          offset,
          va);
        continue;
      }

      {
        auto const e = i->second.back();
        HADESMEM_DETAIL_TRACE_FORMAT_W(
          L"Found matching VA. Logging last entry only. Offset: [%08X]. VA: "
          L"[%p]. Module: [%s]. Name: [%hs]. Ordinal: [%lu]. ByName: [%d].",
          offset,
          va,
          e.module_->name_.c_str(),
          e.name_.c_str(),
          e.ordinal_,
          e.by_name_);
      }

      auto const rva =
        FileOffsetToRva(local_process, pe_file_new, static_cast<DWORD>(offset));

      HADESMEM_DETAIL_TRACE_FORMAT_A("RVA: [%08lX].", rva);

      // Only fixup page aligned VAs if they are adjacent to another fixup,
      // because it would be too risky to reconstruct them otherwise (high
      // chance of false positives when scanning for references).
      // TODO: Add a config flag to control this behavior.
      if (!(reinterpret_cast<std::uintptr_t>(va) % 0x1000) && !fixup_adjacent)
      {
        auto const next_va = *reinterpret_cast<void**>(p + 8);
        auto const next_e = export_map.find(next_va);
        if (next_e == std::end(export_map))
        {
          HADESMEM_DETAIL_TRACE_A("WARNING! Skipping page aligned VA.");
          continue;
        }
      }

      HADESMEM_DETAIL_ASSERT(fixup_map.find(rva) == std::end(fixup_map));

      // Modules are sorted by priority. Lowest to highest.
      auto& fixup_export = fixup_map[rva];
      fixup_export = &i->second.back();

      // Try and match to use the same module as the previous adjacent fixup
      // if possible (there could be a different match because of forwarded
      // exports).
      // TODO: Handle the case where the first import in a list is to a
      // forwarded export. In this case we may detect the initial module wrong
      // (the rest of the list will still be correct, but the first import
      // will get its own descriptor to a different module).
      auto const prev_rva = rva - static_cast<DWORD>(sizeof(void*));
      auto const prev_fixup_iter = fixup_map.find(prev_rva);
      auto const prev_module_base =
        prev_fixup_iter == std::end(fixup_map)
          ? nullptr
          : prev_fixup_iter->second->module_->pe_file_.GetBase();
      if (prev_module_base)
      {
        for (auto& e : i->second)
        {
          if (fixup_export != &e &&
              e.module_->pe_file_.GetBase() == prev_module_base)
          {
            HADESMEM_DETAIL_TRACE_FORMAT_W(
              L"Adjusting fixup to match module of "
              L"previous fixup. Previous: [%s]. "
              L"Current: [%s].",
              fixup_export->module_->name_.c_str(),
              e.module_->name_.c_str());
            fixup_export = &e;
            break;
          }
        }
      }

      fixup_adjacent = true;
      p += sizeof(void*) - 4;
    }

    if (fixup_map.empty())
    {
      HADESMEM_DETAIL_TRACE_A("WARNING! Empty import reconstruction list.");
    }

    return fixup_map;
  }

  void CopyHeaders(std::vector<std::uint8_t>& raw_new,
                   NtHeaders const& nt_headers,
                   std::size_t pe_size,
                   bool has_disk_headers,
                   std::vector<std::uint8_t> const& raw,
                   std::vector<char> const& pe_file_disk_data)
  {
    HADESMEM_DETAIL_TRACE_A("Copying headers.");

    // TODO: Fix this hack properly. We reserve plenty of size in the buffer so
    // when we open the PE file object later pointing to the buffer, if we add
    // more data to the buffer it won't be reallocated.
    raw_new.reserve(pe_size * 5);
    auto const headers_buf_beg =
      has_disk_headers
        ? reinterpret_cast<std::uint8_t const*>(pe_file_disk_data.data())
        : raw.data();
    std::copy(headers_buf_beg,
              headers_buf_beg + nt_headers.GetSizeOfHeaders(),
              raw_new.data());
  }

  std::tuple<bool, std::vector<char>, std::unique_ptr<PeFile>>
    GetPeFileHeaders(bool has_disk_headers,
                     std::wstring const& region_path,
                     Process const& local_process,
                     PeFile const& pe_file)
  {
    if (!has_disk_headers)
    {
      return std::make_tuple(
        false, std::vector<char>(), std::make_unique<PeFile>(pe_file));
    }

    try
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Using disk headers. Path: [%s].",
                                     region_path.c_str());

      // TODO: Disable WoW64 FS redirection.

      // TODO: Support not failing here if we don't have file headers, because
      // we want to support dumping manually mapped PE files with headers.
      // (Sounds like it defeats the purpose but it's more common than you
      // might think...)

      std::vector<char> pe_file_disk_data = PeFileToBuffer(region_path);
      auto pe_file_disk =
        std::make_unique<PeFile>(local_process,
                                 pe_file_disk_data.data(),
                                 PeFileType::kData,
                                 static_cast<DWORD>(pe_file_disk_data.size()));
      return std::make_tuple(
        true, std::move(pe_file_disk_data), std::move(pe_file_disk));
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_A(
        "WARNING! Error attempting to open disk headers.");
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());

      return std::make_tuple(
        false, std::vector<char>(), std::make_unique<PeFile>(pe_file));
    }
  }

  void DumpSingleModule(void* base, ModuleLight const* m = nullptr)
  {
    auto const& process_info = process_light_;
    auto const& modules = process_info.modules_;
    auto const& export_map = process_info.export_map_;

    auto const module_iter = std::find_if(
      std::begin(modules), std::end(modules), [&](ModuleLight const& m) {
        return m.pe_file_.GetBase() == base;
      });

    if (module_iter == std::end(modules))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Failed to find target module."));
    }

    m = &*module_iter;

    auto const pe_size = m->pe_file_.GetSize();

    HADESMEM_DETAIL_TRACE_FORMAT_W(
      L"Starting module dumping. Name: [%s]. Base: [%p]. Size: [%X].",
      m->name_.c_str(),
      base,
      pe_size);

    auto raw = ReadVectorEx<std::uint8_t>(
      *process_, base, pe_size, ReadFlags::kZeroFillReserved);
    Process const local_process(::GetCurrentProcessId());
    PeFile const pe_file(local_process,
                         raw.data(),
                         PeFileType::kImage,
                         static_cast<DWORD>(raw.size()));

    bool has_disk_headers = !!(flags_ & DumpFlags::kUseDiskHeaders);

    auto const& region_path = m->path_;
    if (region_path.empty())
    {
      HADESMEM_DETAIL_TRACE_A(
        "WARNING! No region path, not attempting to use disk headers.");
      has_disk_headers = false;
    }

    std::vector<char> pe_file_disk_data;
    std::unique_ptr<PeFile> pe_file_headers_ptr;
    std::tie(has_disk_headers, pe_file_disk_data, pe_file_headers_ptr) =
      GetPeFileHeaders(has_disk_headers, region_path, local_process, pe_file);

    PeFile const& pe_file_headers = *pe_file_headers_ptr;
    NtHeaders nt_headers(local_process, pe_file_headers);

    std::vector<std::uint8_t> raw_new(nt_headers.GetSizeOfHeaders());
    CopyHeaders(
      raw_new, nt_headers, pe_size, has_disk_headers, raw, pe_file_disk_data);

    auto const section_datas =
      CopySectionData(local_process, pe_file_headers, nt_headers, raw_new, raw);
    raw_new.reserve(raw_new.size() * 2);

    const auto dword_max = (std::numeric_limits<DWORD>::max)();
    HADESMEM_DETAIL_ASSERT(raw_new.size() < dword_max);
    HADESMEM_DETAIL_ASSERT(raw_new.capacity() < dword_max);
    PeFile const pe_file_new(local_process,
                             raw_new.data(),
                             PeFileType::kData,
                             static_cast<DWORD>(raw_new.size()));
    auto const raw_new_capacity = raw_new.capacity();

    std::unique_ptr<PeFile> pe_file_none;
    auto nt_headers_new =
      FixNtHeaders(local_process,
                   pe_file_new,
                   base,
                   has_disk_headers,
                   (has_disk_headers ? pe_file_headers_ptr : pe_file_none));

    SectionList sections_new(local_process, pe_file_new);
    FixSectionHeaders(sections_new, section_datas);

    if (!!(flags_ & DumpFlags::kReconstructImports))
    {
      HADESMEM_DETAIL_TRACE_A("Reconstructing imports.");

      auto const can_add_new_section =
        TryAddNewSection(local_process, pe_file_new, nt_headers_new, raw_new);

      std::vector<Section> sections_vec;
      std::copy(std::begin(sections_new),
                std::end(sections_new),
                std::back_inserter(sections_vec));

      HADESMEM_DETAIL_TRACE_A("Getting last section info.");

      auto& last_section = sections_vec[sections_vec.size() - 1];

      auto const old_size = last_section.GetSizeOfRawData();
      auto const old_section_end =
        can_add_new_section
          ? 0
          : static_cast<DWORD>(RoundUp(last_section.GetVirtualSize(),
                                       nt_headers_new.GetSectionAlignment()));

      auto const fixup_map = DoMemoryScan(
        raw_new, export_map, base, pe_size, local_process, pe_file_new);

      auto coalesced_fixup_map = CoalesceImportDescriptors(fixup_map);

      // TODO: Figure out why IDA doesn't like our import tables sometimes. It
      // shows the import fine in the disassembly view (including name, xrefs,
      // etc) but not in the imports tab. It seems to happen when we have
      // multiple import descriptors for the same library name, because removing
      // the additional entries causes the missing ones to show up... (May need
      // to disable import filtering to trigger this bug depending on the
      // target.)

      auto const filtered_fixup_map = FilterImports(coalesced_fixup_map);

      auto const old_coalesced_fixup_map = coalesced_fixup_map;
      TranslateFilteredImports(coalesced_fixup_map, filtered_fixup_map);

      ReaddFilteredImports(old_coalesced_fixup_map, coalesced_fixup_map);

      // TODO: Try and sort our import directory the same way MSVC would.

      std::vector<char> import_directories_buf;
      DWORD descriptors_cur = 0;
      BuildRawImportDirectoriesBuffer(local_process,
                                      pe_file_headers,
                                      coalesced_fixup_map,
                                      last_section,
                                      old_section_end,
                                      import_directories_buf,
                                      pe_size,
                                      descriptors_cur);

      AddExistingImportDirs(local_process,
                            pe_file_headers,
                            coalesced_fixup_map,
                            import_directories_buf,
                            descriptors_cur);

      auto const new_size = UpdateLastSectionSize(
        import_directories_buf, nt_headers_new, old_section_end, last_section);

      WriteImportDirectories(raw_new,
                             last_section,
                             old_section_end,
                             raw_new_capacity,
                             new_size,
                             old_size,
                             import_directories_buf);

      // TODO: Before we were fixing section sizes correctly above, the last
      // section data was truncated and we didn't appear to detect it properly
      // in the dump tool. Add support for this somewhere.

      SetDataDirectories(
        nt_headers_new, last_section, old_section_end, descriptors_cur);

      FixHeadersFinal(last_section, nt_headers, nt_headers_new);
    }
    else
    {
      FixImports(local_process, pe_file, pe_file_new);
    }

    // TODO: Write to a new sub-directory each time (e.g.
    // pe_dumps\foo.exe\1234\1).
    WriteDumpFile(*process_,
                  PtrToHexString<wchar_t>(base) + L"_" + m->name_,
                  raw_new.data(),
                  raw_new.size(),
                  L"pe_dumps");
  }

  void WriteImportDirectories(std::vector<std::uint8_t>& raw_new,
                              Section const& last_section,
                              DWORD old_section_end,
                              std::size_t raw_new_capacity,
                              DWORD new_size,
                              DWORD old_size,
                              std::vector<char> const& import_directories_buf)
  {
    HADESMEM_DETAIL_TRACE_A("Adding new import directories to file.");

    auto const prev_virt_end =
      last_section.GetPointerToRawData() + old_section_end;
    HADESMEM_DETAIL_TRACE_FORMAT_A("Expanding last section to its previous "
                                   "virtual size. New End: [0x%lX].",
                                   prev_virt_end);
    raw_new.resize(prev_virt_end);

    (void)raw_new_capacity;
    HADESMEM_DETAIL_ASSERT(raw_new.capacity() == raw_new_capacity);

    auto const new_end = raw_new.size() + (new_size - old_size);
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "Appending new data after old virtual end. New End: [0x%IX].", new_end);
    raw_new.reserve(new_end);

    HADESMEM_DETAIL_ASSERT(raw_new.capacity() == raw_new_capacity);

    std::copy(std::begin(import_directories_buf),
              std::end(import_directories_buf),
              std::back_inserter(raw_new));

    HADESMEM_DETAIL_ASSERT(raw_new.capacity() == raw_new_capacity);

    auto const new_raw_size =
      last_section.GetPointerToRawData() + last_section.GetSizeOfRawData();
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "Expanding to section aligned size. New End: [0x%lX].", new_raw_size);
    raw_new.resize(new_raw_size);

    HADESMEM_DETAIL_ASSERT(raw_new.capacity() == raw_new_capacity);
  }

  std::map<DWORD, std::vector<ExportLight const*>> CoalesceImportDescriptors(
    std::map<DWORD, ExportLight const*> const& fixup_map)
  {
    HADESMEM_DETAIL_TRACE_A("Coalescing import descriptors.");

    std::map<DWORD, std::vector<ExportLight const*>> coalesced_fixup_map;
    DWORD cur_rva_base = 0;
    void* prev_module_base = nullptr;
    DWORD prev_rva = 0;
    for (auto const& f : fixup_map)
    {
      auto const cur_module_base = f.second->module_->pe_file_.GetBase();
      if (!prev_rva || f.first != prev_rva + sizeof(void*) ||
          cur_module_base != prev_module_base)
      {
        cur_rva_base = f.first;
        prev_module_base = cur_module_base;
      }

      coalesced_fixup_map[cur_rva_base].emplace_back(f.second);

      prev_rva = f.first;
    }

    return coalesced_fixup_map;
  }

  std::map<void*, std::pair<DWORD, std::vector<ExportLight const*>>>
    FilterImports(std::map<DWORD, std::vector<ExportLight const*>> const&
                    coalesced_fixup_map)
  {
    HADESMEM_DETAIL_TRACE_A("Filtering imports.");

    // Filter out invalid entries. Very basic algorithm right now.
    // TODO: Improve the algorithm for detecting which thunks are valid and
    // which are not.
    // TODO: Add an 'aggressive' switch to disable this pass.
    // TODO: Ideally the user would be able to filter these manually and we
    // could turn on aggressive mode by default? (The config switch would still
    // need to stay in case the user wanted to disable aggressive mode on a
    // binary where it caused large amounts of FPs.)
    std::map<void*, std::pair<DWORD, std::vector<ExportLight const*>>>
      filtered_fixup_map;
    for (auto const& fixup : coalesced_fixup_map)
    {
      auto const module_base = fixup.second.back()->module_->pe_file_.GetBase();
      auto& filtered = filtered_fixup_map[module_base];
      if (!filtered.first || filtered.second.size() < fixup.second.size())
      {
        filtered = fixup;
      }
    }

    return filtered_fixup_map;
  }

  void TranslateFilteredImports(
    std::map<DWORD, std::vector<ExportLight const*>>& coalesced_fixup_map,
    std::map<void*, std::pair<DWORD, std::vector<ExportLight const*>>> const&
      filtered_fixup_map)
  {
    HADESMEM_DETAIL_TRACE_A("Translating filtered imports.");

    coalesced_fixup_map.clear();
    for (auto const& fixup : filtered_fixup_map)
    {
      coalesced_fixup_map[fixup.second.first] = fixup.second.second;
    }
  }

  void ReaddFilteredImports(
    std::map<DWORD, std::vector<ExportLight const*>> const&
      old_coalesced_fixup_map,
    std::map<DWORD, std::vector<ExportLight const*>>& coalesced_fixup_map)
  {
    HADESMEM_DETAIL_TRACE_A("Re-adding incorrectly filtered imports.");

    for (auto iter = std::begin(old_coalesced_fixup_map),
              prev = std::end(old_coalesced_fixup_map);
         iter != std::end(old_coalesced_fixup_map);
         prev = iter++)
    {
      // If it's already there we don't need to do anything.
      if (coalesced_fixup_map.find(iter->first) !=
          std::end(coalesced_fixup_map))
      {
        continue;
      }

      auto next = std::next(iter);
      auto const cur_end = (iter->first + iter->second.size() * sizeof(void*));
      auto const prev_end =
        prev != std::end(old_coalesced_fixup_map)
          ? prev->first + prev->second.size() * sizeof(void*)
          : 0;
      auto const kAlign = 0x10;
      if ((next != std::end(old_coalesced_fixup_map) &&
           cur_end + kAlign >= next->first) ||
          (prev_end && prev_end + kAlign >= iter->first))
      {
        HADESMEM_DETAIL_TRACE_A("Re-inserting incorrectly filtered import.");
        coalesced_fixup_map[iter->first] = iter->second;
      }
    }
  }

  void BuildRawImportDirectoriesBuffer(
    Process const& local_process,
    PeFile const& pe_file_headers,
    std::map<DWORD, std::vector<ExportLight const*>> const& coalesced_fixup_map,
    Section const& last_section,
    DWORD old_section_end,
    std::vector<char>& import_directories_buf,
    std::size_t size,
    DWORD& descriptors_cur)
  {
    HADESMEM_DETAIL_TRACE_A("Building raw import directories buffer.");

    // Build raw import dirs.
    ImportDirList const import_dirs(local_process, pe_file_headers);
    auto const descriptors_size = static_cast<DWORD>(
      sizeof(IMAGE_IMPORT_DESCRIPTOR) *
      (std::distance(std::begin(import_dirs), std::end(import_dirs)) +
       coalesced_fixup_map.size() + 1));
    auto const descriptors_end_rva =
      last_section.GetVirtualAddress() + old_section_end + descriptors_size;
    import_directories_buf.resize(descriptors_size);
    // TODO: Fix this hack (see above for the same thing being done to
    // raw_new).
    import_directories_buf.reserve(size * 2);

    std::map<std::wstring, DWORD> module_name_rvas;
    std::map<std::string, DWORD> import_by_name_rvas;

    std::size_t num_imports = 0;

    // TODO: Don't duplicate entries from existing IAT (above). Need to do
    // more than just check the first thunk though as the array size could
    // differ (packed could have an on-disk IAT size of 1, but it leaves
    // enough space to expand that in memory).
    for (auto const& va_map : coalesced_fixup_map)
    {
      auto const& module_name =
        ToUpperOrdinal(va_map.second.back()->module_->name_);
      auto& module_name_rva = module_name_rvas[module_name];
      if (!module_name_rva)
      {
        module_name_rva = descriptors_end_rva +
                          static_cast<DWORD>(import_directories_buf.size()) -
                          descriptors_size;

        auto const module_name_narrow = WideCharToMultiByte(module_name);
        std::copy(std::begin(module_name_narrow),
                  std::end(module_name_narrow),
                  std::back_inserter(import_directories_buf));
        import_directories_buf.push_back('\0');
      }

      DWORD const thunks_rva =
        descriptors_end_rva +
        static_cast<DWORD>(import_directories_buf.size()) - descriptors_size;
      auto const thunks_offset = import_directories_buf.size();
      import_directories_buf.resize(
        thunks_offset + sizeof(IMAGE_THUNK_DATA) * (va_map.second.size() + 1));
      auto thunks = reinterpret_cast<PIMAGE_THUNK_DATA>(
        &import_directories_buf[thunks_offset]);

      IMAGE_IMPORT_DESCRIPTOR descriptor{};
      // TODO: Flag this.
      descriptor.OriginalFirstThunk = thunks_rva;
      descriptor.Name = module_name_rva;
      descriptor.FirstThunk = va_map.first;
      std::copy(reinterpret_cast<char const*>(&descriptor),
                reinterpret_cast<char const*>(&descriptor) + sizeof(descriptor),
                &import_directories_buf[descriptors_cur]);

      descriptors_cur += sizeof(IMAGE_IMPORT_DESCRIPTOR);

      auto cur_rva = descriptor.FirstThunk;
      for (auto const& e : va_map.second)
      {
        ProcessImpRecEntry(cur_rva,
                           module_name,
                           e,
                           import_by_name_rvas,
                           descriptors_end_rva,
                           import_directories_buf,
                           descriptors_size,
                           thunks,
                           num_imports);
      }
    }

    HADESMEM_DETAIL_TRACE_FORMAT_A("Number of Scanned Imports: [%Iu].",
                                   num_imports);
  }

  void ProcessImpRecEntry(DWORD& cur_rva,
                          std::wstring const& module_name,
                          ExportLight const* e,
                          std::map<std::string, DWORD>& import_by_name_rvas,
                          DWORD descriptors_end_rva,
                          std::vector<char>& import_directories_buf,
                          DWORD descriptors_size,
                          PIMAGE_THUNK_DATA& thunks,
                          std::size_t& num_imports)
  {
    (void)module_name;
    HADESMEM_DETAIL_TRACE_FORMAT_W(
      L"Processing import reconstruction entry. RVA: [%08lX]. Module: "
      L"[%s]. Name: [%hs]. Ordinal: [%lu]. ByName: [%d].",
      cur_rva,
      module_name.c_str(),
      e->name_.c_str(),
      e->ordinal_,
      e->by_name_);

    if (e->by_name_)
    {
      auto& import_by_name_rva = import_by_name_rvas[e->name_];
      if (!import_by_name_rva)
      {
        import_by_name_rva = descriptors_end_rva +
                             static_cast<DWORD>(import_directories_buf.size()) -
                             descriptors_size;

        // TODO: Set Hint.
        std::vector<char> import_by_name_buf(
          sizeof(IMAGE_IMPORT_BY_NAME::Hint) + e->name_.size() + 1);
        auto const import_by_name =
          reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(import_by_name_buf.data());
        std::copy(
          std::begin(e->name_), std::end(e->name_), &import_by_name->Name[0]);

        std::copy(std::begin(import_by_name_buf),
                  std::end(import_by_name_buf),
                  std::back_inserter(import_directories_buf));
      }

      thunks->u1.AddressOfData = import_by_name_rva;
    }
    else
    {
      thunks->u1.Ordinal =
        IMAGE_ORDINAL_FLAG | (e->ordinal_ & static_cast<WORD>(-1));
    }

    ++num_imports;
    ++thunks;
    cur_rva += sizeof(void*);
  }

  DWORD UpdateLastSectionSize(std::vector<char>& import_directories_buf,
                              NtHeaders const& nt_headers_new,
                              DWORD old_section_end,
                              Section& last_section)
  {
    HADESMEM_DETAIL_TRACE_A("Updating last section size.");

    // TODO: This probably doesn't need to be section aligned?
    auto const import_dirs_aligned_size = static_cast<DWORD>(RoundUp(
      import_directories_buf.size(), nt_headers_new.GetSectionAlignment()));
    auto const new_size = old_section_end + import_dirs_aligned_size;
    last_section.SetSizeOfRawData(new_size);
    // TODO: Is increasing the virtual size here ever incorrect? (Will be
    // solved by simply using a new section, but that may not always be
    // desired, so we should probably keep this branch too.
    last_section.SetVirtualSize(new_size);
    last_section.UpdateWrite();

    return new_size;
  }

  bool TryAddNewSection(Process const& local_process,
                        PeFile const& pe_file_new,
                        NtHeaders& nt_headers_new,
                        std::vector<std::uint8_t> const& raw_new)
  {
    if (!(flags_ & DumpFlags::kAddNewSection))
    {
      return false;
    }

    hadesmem::Section last_section(
      local_process, pe_file_new, nt_headers_new.GetNumberOfSections() - 1);
    // TODO: Is this correct in all cases?
    auto const new_section_header_beg =
      static_cast<std::uint8_t*>(last_section.GetBase()) +
      sizeof(IMAGE_SECTION_HEADER);
    auto const new_section_header_end =
      new_section_header_beg + sizeof(IMAGE_SECTION_HEADER);
    if (new_section_header_end >
        raw_new.data() + nt_headers_new.GetSizeOfHeaders())
    {
      // TODO: What can we do to handle this? Is it possible to expand the size
      // of the headers safely?
      HADESMEM_DETAIL_TRACE_A("WARNING! No space for new section header.");
      return false;
    }

    HADESMEM_DETAIL_TRACE_A("Adding new section.");

    IMAGE_SECTION_HEADER hadesmem_section{};
    std::memcpy(hadesmem_section.Name, ".hmem", sizeof(".hmem"));
    hadesmem_section.VirtualAddress =
      last_section.GetVirtualAddress() + last_section.GetVirtualSize();
    hadesmem_section.PointerToRawData =
      last_section.GetPointerToRawData() + last_section.GetSizeOfRawData();
    hadesmem_section.Characteristics =
      IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA |
      IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

    std::copy(reinterpret_cast<std::uint8_t*>(&hadesmem_section),
              reinterpret_cast<std::uint8_t*>(&hadesmem_section + 1),
              new_section_header_beg);

    nt_headers_new.SetNumberOfSections(nt_headers_new.GetNumberOfSections() +
                                       1);
    nt_headers_new.UpdateWrite();

    return true;
  }

  void AddExistingImportDirs(
    Process const& local_process,
    PeFile const& pe_file_headers,
    std::map<DWORD, std::vector<ExportLight const*>> const& coalesced_fixup_map,
    std::vector<char>& import_directories_buf,
    DWORD descriptors_cur)
  {
    std::size_t num_existing_imports = 0;
    (void)num_existing_imports;

    HADESMEM_DETAIL_TRACE_A("Adding existing import directories.");
    ImportDirList const import_dirs(local_process, pe_file_headers);
    for (auto const& dir : import_dirs)
    {
      AddSingleExistingImportDir(local_process,
                                 pe_file_headers,
                                 dir,
                                 coalesced_fixup_map,
                                 num_existing_imports,
                                 import_directories_buf,
                                 descriptors_cur);
    }

    descriptors_cur += sizeof(IMAGE_IMPORT_DESCRIPTOR);

    HADESMEM_DETAIL_TRACE_FORMAT_A("Number of Existing Imports: [%Iu].",
                                   num_existing_imports);
  }

  void AddSingleExistingImportDir(
    Process const& local_process,
    PeFile const& pe_file_headers,
    ImportDir const& dir,
    std::map<DWORD, std::vector<ExportLight const*>> const& coalesced_fixup_map,
    std::size_t& num_existing_imports,
    std::vector<char>& import_directories_buf,
    DWORD& descriptors_cur)
  {
    hadesmem::ImportThunkList const import_thunks_ft(
      local_process, pe_file_headers, dir.GetFirstThunk());
    auto const num_fts =
      std::distance(std::begin(import_thunks_ft), std::end(import_thunks_ft));

    // Skip the on-disk directory if the scanned directory is the same size
    // or larger (for packers which put FTs in virtual space and dynamically
    // fill it themselves).
    // TODO: Fix this for the case where the on-disk IAT is larger than the
    // scanned one. In this case we should probably expand the size of the
    // scanned list, but we can't do that here as we've already written it.
    // Need to move this to one phase earlier.
    auto const iter = coalesced_fixup_map.find(dir.GetFirstThunk());
    if (iter != std::end(coalesced_fixup_map) &&
        iter->second.size() >= static_cast<std::size_t>(num_fts))
    {
      return;
    }

    // Skip if the directory has unaligned FTs.
    // TODO: Put this behind a config flag.
    if (!!(dir.GetFirstThunk() & 3))
    {
      return;
    }

    hadesmem::ImportThunkList import_thunks_oft(
      local_process, pe_file_headers, dir.GetOriginalFirstThunk());
    auto const num_ofts =
      std::distance(std::begin(import_thunks_oft), std::end(import_thunks_oft));
    auto const num_new_existing_imports =
      (std::min)(num_fts, num_ofts ? num_ofts : num_fts);
    HADESMEM_DETAIL_TRACE_FORMAT_A("Adding %d existing imports.",
                                   num_new_existing_imports);
    num_existing_imports += num_new_existing_imports;

    auto const p = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(dir.GetBase());
    std::copy(reinterpret_cast<char const*>(p),
              reinterpret_cast<char const*>(p) + sizeof(*p),
              &import_directories_buf[descriptors_cur]);
    descriptors_cur += sizeof(IMAGE_IMPORT_DESCRIPTOR);
  }

  void SetDataDirectories(NtHeaders& nt_headers_new,
                          Section const& last_section,
                          DWORD old_section_end,
                          DWORD descriptors_cur)
  {
    HADESMEM_DETAIL_TRACE_A("Setting import data directory info.");

    nt_headers_new.SetDataDirectoryVirtualAddress(
      PeDataDir::Import, last_section.GetVirtualAddress() + old_section_end);
    nt_headers_new.SetDataDirectorySize(PeDataDir::Import, descriptors_cur);

    // TODO: Instead of searching for and placing individual
    // descriptors/thunks, we should search for the original IAT as a whole.
    // This won't always work vs all packers, but it should probably be the
    // default, and then the current behavior of doing everything from scratch
    // should be moved behind a config flag. (See also the 'aggressive' flag.)
    nt_headers_new.SetDataDirectoryVirtualAddress(PeDataDir::IAT, 0);
    nt_headers_new.SetDataDirectorySize(PeDataDir::IAT, 0);
  }

  void FixHeadersFinal(Section const& last_section,
                       NtHeaders const& nt_headers,
                       NtHeaders& nt_headers_new)
  {
    HADESMEM_DETAIL_TRACE_A("Setting image size.");

    auto const new_image_size = static_cast<DWORD>(
      RoundUp(last_section.GetVirtualAddress() + last_section.GetVirtualSize(),
              nt_headers.GetSectionAlignment()));
    nt_headers_new.SetSizeOfImage(new_image_size);

    // TODO: Fix checksums (with flag).

    // TODO: Strip DOS stub (with flag).

    nt_headers_new.UpdateWrite();
  }

  void FixImports(Process const& local_process,
                  PeFile const& pe_file,
                  PeFile const& pe_file_new) const
  {
    HADESMEM_DETAIL_TRACE_A("Fixing imports.");

    ImportDirList const import_dirs(local_process, pe_file);
    ImportDirList const import_dirs_new(local_process, pe_file_new);
    auto i = std::begin(import_dirs), j = std::begin(import_dirs_new);
    bool thunk_mismatch = false;
    for (; i != std::end(import_dirs) && j != std::end(import_dirs_new);
         ++i, ++j)
    {
      ImportThunkList const import_thunks(
        local_process, pe_file, i->GetOriginalFirstThunk());
      ImportThunkList import_thunks_new(
        local_process, pe_file_new, j->GetFirstThunk());
      auto a = std::begin(import_thunks);
      auto b = std::begin(import_thunks_new);
      for (; a != std::end(import_thunks) && b != std::end(import_thunks_new);
           ++a, ++b)
      {
        b->SetFunction(a->GetFunction());
        b->UpdateWrite();
      }
      thunk_mismatch = thunk_mismatch || ((a != std::end(import_thunks)) ^
                                          (b != std::end(import_thunks_new)));
    }
    bool const dir_mismatch =
      (i != std::end(import_dirs)) ^ (j != std::end(import_dirs));

    if (dir_mismatch)
    {
      HADESMEM_DETAIL_TRACE_A("Mismatch in import dir processing.");
    }

    if (thunk_mismatch)
    {
      HADESMEM_DETAIL_TRACE_A("Mismatch in import thunk processing.");
    }
  }

  // TODO: Support running this on a memory dump (i.e. a module on disk, but in
  // memory image form). Useful for reconstructing modules from crash dumps etc.
  // Also useful for dumping kernel modules.
  // TODO: Support 64-bit dumping 32-bit otherwise some targets will not be
  // possible to dump due to memory pressure.
  // TODO: Add option to not dump system files?
  // TODO: Split this into many smaller types and functions.
  // TODO: Support plugins for import reconstruction.
  // TODO: Disable relocations in dumped modules (just null out data directory
  // entry, don't remove reloc section because it may contain more than just
  // relocs)? Must be behind a config flag.
  // TODO: Rewrite direct imports. (e.g. Themida changes a jump through a
  // function pointer into a direct jump, and inserts a nop to cover the extra
  // byte. We need to be careful when rewriting to not assume that the nop is
  // always before or after the jump, because it is seemingly randomly
  // generated. So, instead of guessing and probably being wrong, we should
  // write a jump to some dynamically generated code we insert into the PE which
  // then in turn does the jump through the IAT. This way we both get our IAT
  // reference and don't need to risk creating a bad dump. Another thing to
  // consider is ensuring that the section we're inserting the code into is
  // executable, and also whether we should add new sections instead of just
  // appending to the last one like we have been so far. New sections would mean
  // we could mark them properly, which could theoretically lead to better
  // disassambly? It's hopeful thinking, but it can't hurt to try.)
  // TODO: Don't trust values in the headers so much.
  // TODO: Add optional OEP scan. Also make it optional whether to adjust the
  // OEP in the headers, or simply add the OEP as a new custom export.
  // TODO: Detect and resolve apphelp shims?
  inline void DumpAllModules()
  {
    auto const& modules = process_light_.modules_;
    for (auto const& m : modules)
    {
      DumpSingleModule(m.pe_file_.GetBase(), &m);
    }
  }

  // Overwatch redirects all entries in the IAT to a dynamically generated stub
  // which performs some rudimentary pointer arithmetic to mask the API being
  // called.
  // 0:000> u 00007FF7170224F0
  // Overwatch+0x1724f0:
  // 00007ff7`170224f0 488d0d6de22f01  lea     rcx,[Overwatch+0x1470764
  // (00007ff7`18320764)] ; "ntdll.dll"
  // 00007ff7`170224f7 ff15cbecef00    call    qword ptr [Overwatch+0x10711c8
  // (00007ff7`17f211c8)]
  // 0:000> u poi(00007ff7`17f211c8)
  // 00000223`c86f00fc 48b8151334b7fe7f0000 mov rax,offset
  // kernel32!FindFirstVolumeW+0x195 (00007ffe`b7341315)
  // 00000223`c86f0106 48057b390000    add     rax,397Bh
  // 00000223`c86f010c ffe0            jmp     rax
  // 0:000> ln 0x00007ffe`b7341315 + 0x3970
  // (00007ffe`b7344c90)   kernel32!GetModuleHandleAStub
  // NOTE: Doesn't work since the last patch, and I won't be updating it for
  // legal reasons. Not going to remove it though because whilst it's not useful
  // for its original purpose it still serves as an example of how to extend the
  // import redirection resolution code.
  // TODO: This should be a plugin/extension/whatever.
  // TODO: Further perf improvements by using the buffer and memory region info
  // we already have in the memory scanning function, rather than duplicating
  // work here (and in other 'extensions').
  void* ResolveRedirectedImportForOverwatch(void* va) const
  {
#if defined(HADESMEM_DETAIL_ARCH_X64)
    try
    {
      auto const stub = static_cast<std::uint8_t*>(va);
      if (!CanExecute(*process_, stub))
      {
        return nullptr;
      }

      const auto stub_buf = ReadVector<std::uint8_t>(*process_, stub, 0x11);

      // mov rax, imm64
      if (stub_buf[0] != 0x48 || stub_buf[1] != 0xB8)
      {
        return nullptr;
      }

      // add rax, imm32
      if (stub_buf[0xA] != 0x48 || stub_buf[0xB] != 0x05)
      {
        return nullptr;
      }

      // jmp rax
      if (stub_buf[0x10] != 0xFF || stub_buf[0x11] != 0xE0)
      {
        return nullptr;
      }

      auto const o = *reinterpret_cast<std::uint8_t* const*>(&stub_buf[2]);
      auto const n = *reinterpret_cast<std::uint32_t const*>(&stub_buf[0xc]);

      return o + n;
    }
    catch (...)
    {
      return nullptr;
    }
#else // #if defined(HADESMEM_DETAIL_ARCH_X64)
    (void)va;
    return nullptr;
#endif
  }

  void* ResolveRedirectedImport(void* va) const
  {
    // Just hardcode one for now. Needs proper plugin support.
    return ResolveRedirectedImportForOverwatch(va);
  }

  Process const* process_{};
  void* base_{};
  std::uint32_t flags_{};
  DWORD oep_{};
  ModuleLight const* m_{};

  ProcessLight process_light_;
};

inline void
  DumpMemoryRegionRaw(Process const& process, void* base, std::size_t size)
{
  HADESMEM_DETAIL_TRACE_A("Reading memory.");

  auto raw = ReadVector<std::uint8_t>(process, base, size);

  WriteDumpFile(
    process, PtrToHexString(base), raw.data(), raw.size(), L"raw_dumps");
}

inline void
  DumpMemory(Process const& process = Process(::GetCurrentProcessId()),
             std::uint32_t flags = DumpFlags::kNone,
             DWORD oep = 0,
             void* module_base = nullptr,
             void* base = nullptr,
             std::size_t size = 0)
{
  if (base && size)
  {
    DumpMemoryRegionRaw(process, base, size);
  }
  else
  {
    // TODO: What happens vs targets which unmap themselves when we specify not
    // to fall back to the original image path (e.g. Overwatch, some malware).
    PeDumper dumper(process, module_base, flags, oep);
    dumper.Dump();
  }
}
}
}
