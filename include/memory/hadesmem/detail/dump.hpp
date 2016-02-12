// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>

#include <windows.h>
#include <psapi.h>

#include <hadesmem/config.hpp>
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

namespace hadesmem
{
namespace detail
{
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

inline std::wstring GetRegionName(Process const& process, void* p) noexcept
{
  try
  {
    std::vector<wchar_t> mapped_file_name(HADESMEM_DETAIL_MAX_PATH_UNICODE);
    if (::GetMappedFileNameW(process.GetHandle(),
                             p,
                             mapped_file_name.data(),
                             static_cast<DWORD>(mapped_file_name.size())))
    {
      auto const region_name =
        static_cast<std::wstring>(mapped_file_name.data());
      return region_name.substr(region_name.rfind(L'\\') + 1);
    }

    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_TRACE_FORMAT_A("WARNING! GetMappedFileNameW failed. "
                                   "Defaulting to base address as string. "
                                   "LastError: [%08lX].",
                                   last_error);

    // TODO: Select extension based off headers (for EXE, DLL and SYS at least).
    return L"unknown_" + PtrToHexString(p) + L".dll";
  }
  catch (...)
  {
    return {};
  }
}

// TODO: Support files mapped from UNC shares? Any other corner cases?
inline std::wstring GetRegionPath(Process const& process, void* p)
{
  try
  {
    std::vector<wchar_t> mapped_file_name(HADESMEM_DETAIL_MAX_PATH_UNICODE);
    if (!::GetMappedFileNameW(process.GetHandle(),
                              p,
                              mapped_file_name.data(),
                              static_cast<DWORD>(mapped_file_name.size())))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "WARNING! GetMappedFileNameW failed. LastError: [%08lX].", last_error);
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
              *(mapped_file_name.data() + device_path_len) == L'\\')
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
    return {};
  }
}

struct ModuleLight
{
  ModuleLight(Process const& process, void* base, std::size_t size)
    : process_{process},
      pe_file_{process_, base, PeFileType::Image, static_cast<DWORD>(size)},
      nt_headers_{process_, pe_file_},
      name_{GetRegionName(process, base)},
      path_{GetRegionPath(process, base)}
  {
    // TODO: Do our best to get module name, path, etc. here. For manually
    // mapped modules we can check the export directory for a module name.
    // Need to be careful though because it could overlap with an existing
    // filename, be an invalid file name, etc. Should probably do some
    // sanitization on the data (e.g. ascii alphanumeric only) and then use it
    // plus the region address or something to ensure it's unique.
    // TODO: Ensure this doesn't overlap with an existing name. We could
    // easily get two different modules with the same name loaded into a
    // process. For example, this happens by default with WoW64, but there are
    // also lots of other valid scenarios.
  }

  Process process_;
  PeFile pe_file_;
  NtHeaders nt_headers_;
  std::wstring name_;
  std::wstring path_;
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

inline ProcessLight BuildExportMap(Process const& process)
{
  HADESMEM_DETAIL_TRACE_A("Getting page size.");

  SYSTEM_INFO sys_info{};
  ::GetSystemInfo(&sys_info);
  auto const page_size = sys_info.dwPageSize;

  ProcessLight process_info;

  HADESMEM_DETAIL_TRACE_A("Building module list.");

  RegionList regions(process);
  for (auto const& region : regions)
  {
    auto const region_beg = static_cast<std::uint8_t*>(region.GetBase());
    if (!region_beg)
    {
      continue;
    }

    auto const region_end = region_beg + region.GetSize();

    // TODO: Verify the pages.

    // TODO: Better validations.
    if (!CanRead(process, region_beg) || IsBadProtect(process, region_beg))
    {
      continue;
    }

    for (auto p = region_beg; p + page_size <= region_end; p += page_size)
    {
      auto const mz = Read<std::array<char, 2>>(process, p);
      if (mz[0] != 'M' || mz[1] != 'Z')
      {
        continue;
      }

      HADESMEM_DETAIL_TRACE_FORMAT_A("Found module candidate. Base: [%p].", p);

      // We potentially over-estimate module size here, but it's probably best
      // to over-estimate in the case where we have no headers, and in the case
      // where we do have headers it's not a big deal because we only use this
      // size to determine the maximum amount of data to read, not the minimum.
      auto const size = GetRegionAllocSize(process, p, true);
      if (size > static_cast<DWORD>(-1))
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                        << ErrorString("Region too large."));
      }

      HADESMEM_DETAIL_TRACE_FORMAT_A("Calculated module size: [%08lX].",
                                     static_cast<DWORD>(size));

      try
      {
        HADESMEM_DETAIL_TRACE_A("Checking for valid headers, and retrieving "
                                "region name and optional path.");
        process_info.modules_.emplace_back(process, p, size);

        HADESMEM_DETAIL_TRACE_FORMAT_W(
          L"Name: [%s]. Path: [%s].",
          process_info.modules_.back().name_.c_str(),
          process_info.modules_.back().path_.c_str());
      }
      catch (std::exception const& /*e*/)
      {
        HADESMEM_DETAIL_TRACE_FORMAT_A("WARNING! Invalid headers. Base: [%p].",
                                       p);
        continue;
      }
    }
  }

  struct ExportLight
  {
    ModuleLight const* module_;

    bool by_name_;
    std::string name_;
    DWORD ordinal_;
  };

  std::map<void*, std::vector<ExportLight>> export_map;

  HADESMEM_DETAIL_TRACE_A("Building export map.");

  // TODO: Only do this if import reconstruction is enabled.
  for (auto const& m : process_info.modules_)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Module: [%s].", m.name_.c_str());

    // TODO: Use on disk headers here if we have the option? Or both?
    // TODO: If the module is loaded in our own process we could theoretically
    // enumerate exports locally?
    ExportList exports(m.process_, m.pe_file_);
    for (auto const& e : exports)
    {
      auto va = e.GetVa();
      if (e.IsForwarded())
      {
        HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
          "Got forwarded export. Name: [%s]. Ordinal: [%d]. Forwarder: [%s].",
          e.GetName().c_str(),
          e.GetProcedureNumber(),
          e.GetForwarder().c_str());

        try
        {
          // TODO: Handle API sets.
          // API-
          // EXT-
          // TODO: Are there other prefixes?
          // TODO: Do they live anywhere other than the system directory?
          // TODO: Parse the API set schema table instead for extra accuracy (at
          // the expense of being tied to particular windows versions)?

          va = GetProcAddressFromExport(process, e);
        }
        catch (...)
        {
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
        HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
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

      export_map[va].emplace_back(
        ExportLight{&m, e.ByName(), e.GetName(), e.GetProcedureNumber()});
    }
  }

  return process_info;
}

inline void DumpSingleModule(Process const& process,
                             void* base,
                             bool use_disk_headers,
                             bool reconstruct_imports,
                             bool add_new_section,
                             DWORD oep,
                             ModuleLight const* m = nullptr)
{
  // TODO: Don't duplicate this work when doing a full module dump. Do it once
  // at the start and then reuse it for each module.
  auto const process_info = BuildExportMap(process);

  auto& modules = process_info.modules_;
  auto& export_map = process_info.export_map_;

  auto const module_iter = std::find_if(std::begin(modules),
                                        std::end(modules),
                                        [&](ModuleLight const& m)
                                        {
                                          return m.pe_file_.GetBase() == base;
                                        });

  if (module_iter == std::end(modules))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("Failed to find target module."));
  }

  m = &*module_iter;

  HADESMEM_DETAIL_TRACE_A("Starting module dumping.");

  auto const size = m->pe_file_.GetSize();

  HADESMEM_DETAIL_TRACE_A("Reading memory.");

  auto raw = ReadVectorEx<std::uint8_t>(
    process, base, size, ReadFlags::kZeroFillReserved);
  Process const local_process(::GetCurrentProcessId());
  PeFile const pe_file(local_process,
                       raw.data(),
                       PeFileType::Image,
                       static_cast<DWORD>(raw.size()));

  bool has_disk_headers = use_disk_headers;

  auto const& region_path = m->path_;
  if (region_path.empty())
  {
    HADESMEM_DETAIL_TRACE_A(
      "WARNING! No region path, not attempting to use disk headers.");
    has_disk_headers = false;
  }

  // TODO: Don't hard-fail if we request disk headers but can't get them?

  std::vector<char> pe_file_disk_data;
  std::unique_ptr<PeFile> pe_file_disk;
  if (has_disk_headers)
  {
    try
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Using disk headers. Path: [%s].",
                                     region_path.c_str());

      // TODO: Disable WoW64 FS redirection.

      // TODO: Support not failing here if we don't have file headers, because
      // we want to support dumping manually mapped PE files with headers.
      // (Sounds like it defeats the purpose but it's more common than you
      // might think...)
      pe_file_disk_data = PeFileToBuffer(region_path);
      pe_file_disk =
        std::make_unique<PeFile>(local_process,
                                 pe_file_disk_data.data(),
                                 PeFileType::Data,
                                 static_cast<DWORD>(pe_file_disk_data.size()));
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_A(
        "WARNING! Error attempting to open disk headers.");
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());

      has_disk_headers = false;
    }
  }

  PeFile const& pe_file_headers = has_disk_headers ? *pe_file_disk : pe_file;
  NtHeaders nt_headers(local_process, pe_file_headers);

  HADESMEM_DETAIL_TRACE_A("Copying headers.");

  std::vector<std::uint8_t> raw_new(nt_headers.GetSizeOfHeaders());
  // TODO: Fix this hack properly. We reserve plenty of size in the buffer so
  // when we open the PE file object later pointing to the buffer, if we add
  // more data to the buffer it won't be reallocated.
  raw_new.reserve(size * 5);
  auto const headers_buf_beg =
    has_disk_headers
      ? reinterpret_cast<std::uint8_t const*>(pe_file_disk_data.data())
      : raw.data();
  std::copy(headers_buf_beg,
            headers_buf_beg + nt_headers.GetSizeOfHeaders(),
            raw_new.data());

  HADESMEM_DETAIL_TRACE_A("Copying section data.");

  SectionList const sections(local_process, pe_file_headers);
  struct SectionData
  {
    DWORD raw_data_;
    DWORD raw_size_;
    DWORD virtual_size_;
  };
  std::vector<SectionData> section_datas;
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

    auto const section_size = static_cast<DWORD>(
      RoundUp((std::max)(section.GetVirtualSize(), section.GetSizeOfRawData()),
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

    // TODO: Actually validate we're not going to read outside of the buffer?
    // TODO: Everywhere else in this function needs hardening too.

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
         p > raw_new.data() + ptr_raw_data_new + section.GetSizeOfRawData();
         p -= sizeof(std::uint32_t))
    {
      if (*reinterpret_cast<std::uint32_t*>(p))
      {
        break;
      }

      raw_section_size -= sizeof(std::uint32_t);
    }

    raw_section_size = static_cast<DWORD>(
      RoundUp(raw_section_size, nt_headers.GetFileAlignment()));

    section_datas.back().raw_size_ = raw_section_size;
  }

  HADESMEM_DETAIL_ASSERT(raw_new.size() < (std::numeric_limits<DWORD>::max)());
  PeFile const pe_file_new(local_process,
                           raw_new.data(),
                           PeFileType::Data,
                           static_cast<DWORD>(raw_new.size()));
  auto const raw_new_capacity = raw_new.capacity();

  HADESMEM_DETAIL_TRACE_A("Fixing NT headers.");

  NtHeaders nt_headers_new(local_process, pe_file_new);

  // TODO: Add flag.
  auto const image_base_new =
    static_cast<ULONGLONG>(reinterpret_cast<ULONG_PTR>(base));
  HADESMEM_DETAIL_TRACE_FORMAT_A("ImageBase: [%016llX] -> [%016llX].",
                                 nt_headers_new.GetImageBase(),
                                 image_base_new);
  nt_headers_new.SetImageBase(image_base_new);

  if (oep)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("AddressOfEntryPoint: [%08lX] -> [%08lX].",
                                   nt_headers_new.GetAddressOfEntryPoint(),
                                   oep);
    nt_headers_new.SetAddressOfEntryPoint(oep);
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

  HADESMEM_DETAIL_TRACE_A("Fixing section headers.");

  SectionList sections_new(local_process, pe_file_new);
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

  // Expand the last section to hold our new import directories.
  // TODO: Add a new section instead of expanding the last section.
  if (reconstruct_imports)
  {
    HADESMEM_DETAIL_TRACE_A("Reconstructing imports.");

    if (add_new_section)
    {
      hadesmem::Section last_section(
        local_process, pe_file_new, nt_headers_new.GetNumberOfSections() - 1);
      // TODO: Is this correct in all cases?
      auto const new_section_header_beg =
        static_cast<std::uint8_t*>(last_section.GetBase()) +
        sizeof(IMAGE_SECTION_HEADER);
      auto const new_section_header_end =
        new_section_header_beg + sizeof(IMAGE_SECTION_HEADER);
      if (new_section_header_end <=
          raw_new.data() + nt_headers_new.GetSizeOfHeaders())
      {
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

        nt_headers_new.SetNumberOfSections(
          nt_headers_new.GetNumberOfSections() + 1);
        nt_headers_new.UpdateWrite();
      }
      else
      {
        HADESMEM_DETAIL_TRACE_A("WARNING! No space for new section.");
        add_new_section = false;
      }
    }

    std::vector<Section> sections_vec;
    std::copy(std::begin(sections_new),
              std::end(sections_new),
              std::back_inserter(sections_vec));

    HADESMEM_DETAIL_TRACE_A("Getting last section info.");

    auto& last_section = sections_vec[sections_vec.size() - 1];

    auto const old_size = last_section.GetSizeOfRawData();
    auto const old_section_end =
      add_new_section
        ? 0
        : static_cast<DWORD>(RoundUp(last_section.GetVirtualSize(),
                                     nt_headers_new.GetSectionAlignment()));

    HADESMEM_DETAIL_TRACE_A("Performing memory scan.");

    // TODO: Support other scanning algorithms.
    // TODO: Be smarter about deciding which imports are legitimate and which
    // are false positives.
    std::map<DWORD, ExportLight const*> fixup_map;
    bool fixup_adjacent = false;
    for (auto p = raw_new.data(); p < raw_new.data() + raw_new.size() - 3;
         p += 4)
    {
      auto const va = *reinterpret_cast<void**>(p);
      auto const i = export_map.find(va);
      if (i == std::end(export_map))
      {
        fixup_adjacent = false;
        continue;
      }

      if (va >= base && va <= static_cast<std::uint8_t*>(base) + size)
      {
        HADESMEM_DETAIL_TRACE_FORMAT_A(
          "WARNING! Skipping VA in current module. Pointer: [%p].", p);
        continue;
      }

      {
        auto const e = i->second.back();
        HADESMEM_DETAIL_TRACE_NOISY_FORMAT_W(
          L"Found matching VA. Logging last entry only. Pointer: [%p]. VA: "
          L"[%p]. Module: [%s]. Name: [%hs]. Ordinal: [%lu]. ByName: [%d].",
          p,
          va,
          e.module_->name_.c_str(),
          e.name_.c_str(),
          e.ordinal_,
          e.by_name_);
      }

      auto const offset =
        static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(p) -
                           reinterpret_cast<std::uintptr_t>(raw_new.data()));

      auto const rva =
        FileOffsetToRva(local_process, pe_file_new, static_cast<DWORD>(offset));

      HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
        "Offset: [%08lX]. RVA: [%08lX].", offset, rva);

      // Only fixup page aligned VAs if they are adjacent to another fixup,
      // because it would be too risky to reconstruct them otherwise (high
      // chance of false positives when scanning for references).
      // TODO: Also check the next fixup, not just the previous one. We could
      // be adjacent in either direction and it still counts.
      // TODO: Add a 'skip list' and log any matches we find later for
      // debugging purposes (or cases where advanced users may want to
      // manually apply some fixes).
      // TODO: Add an override switch?
      if (!(reinterpret_cast<std::uintptr_t>(va) % 0x1000) && !fixup_adjacent)
      {
        auto const next_va = *reinterpret_cast<void**>(p + 8);
        auto const next_e = export_map.find(next_va);
        if (next_e == std::end(export_map))
        {
          HADESMEM_DETAIL_TRACE_FORMAT_A(
            "WARNING! Skipping page aligned VA. Pointer: [%p].", p);
          continue;
        }
      }

      if (fixup_map.find(rva) != std::end(fixup_map))
      {
        HADESMEM_DETAIL_TRACE_FORMAT_A(
          "WARNING! Skipping duplicate RVA. Pointer: [%p].", p);
        continue;
      }

      // Default to the first one seen.
      // TODO: This is just a placeholder value. We need to think about what
      // the best thing to do here is... (Perhaps this is it?)
      auto& fixup_export = fixup_map[rva];
      fixup_export = &i->second.front();

      // Try and match to use the same module as the previous adjacent fixup
      // if possible (there could be a different match because of forwarded
      // exports).
      // TODO: Handle the case where the first import in a list is to a
      // forwarded export. In this case we may detect the initial module wrong
      // (the rest of the list will still be correct, but the first import
      // will get its own descriptor to a different module).
      auto const prev_rva = rva - static_cast<DWORD>(sizeof(void*));
      auto const prev_fixup_iter = fixup_map.find(prev_rva);
      if (auto const prev_module_base =
            prev_fixup_iter == std::end(fixup_map)
              ? nullptr
              : prev_fixup_iter->second->module_->pe_file_.GetBase())
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

    // TODO: Sort import descriptors by module name.

    HADESMEM_DETAIL_TRACE_A("Coalescing import descriptors.");

    std::map<DWORD, std::vector<ExportLight const*>> coalesced_fixup_map;
    DWORD cur_rva_base = 0;
    DWORD prev_rva = 0;
    for (auto const& f : fixup_map)
    {
      if (!prev_rva || f.first != prev_rva + sizeof(void*))
      {
        cur_rva_base = f.first;
      }

      coalesced_fixup_map[cur_rva_base].emplace_back(f.second);

      prev_rva = f.first;
    }

    // TODO: Now that we have a full list, try and identify which imports are
    // 'real' and which to ignore (add an extra flag to enable more aggressive
    // import recovery).
    (void)0;

    // TODO: Figure out why IDA doesn't like our import tables sometimes. It
    // shows the import fine in the disassembly view (including name, xrefs,
    // etc) but not in the imports tab. It seems to happen when we have
    // multiple impor descriptors for the same library name, because removing
    // the additional entries causes the missing ones to show up...

    HADESMEM_DETAIL_TRACE_A("Removing redundant import descriptors.");

    ImportDirList import_dirs(local_process, pe_file_headers);
    for (auto const& id : import_dirs)
    {
      auto const iter = coalesced_fixup_map.find(id.GetFirstThunk());
      if (iter != std::end(coalesced_fixup_map))
      {
        ImportThunkList import_thunks(
          local_process, pe_file_headers, id.GetFirstThunk());
        auto const num_thunks =
          std::distance(std::begin(import_thunks), std::end(import_thunks));
        HADESMEM_DETAIL_ASSERT(num_thunks >= 0);
        if (static_cast<std::size_t>(num_thunks) == iter->second.size())
        {
          HADESMEM_DETAIL_TRACE_A("Found perfect match.");
        }
      }
    }

    HADESMEM_DETAIL_TRACE_A("Building raw import directories buffer.");

    // Build raw import dirs.
    std::vector<char> import_directories_buf;
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
    DWORD descriptors_cur = 0;

    std::map<std::wstring, DWORD> module_name_rvas;
    std::map<std::string, DWORD> import_by_name_rvas;

    std::size_t num_imports = 0;

    // TODO: Don't duplicate entries from existing IAT (above). Need to do
    // more than just check the first thunk though as the array size could
    // differ (packed could have an on-disk IAT size of 1, but it leaves
    // enough space to expand that in memory).
    for (auto const& va_map : coalesced_fixup_map)
    {
      // TODO: Make this case insensitive? Need to think about it and make
      // sure it's a reasonable thing to do. Probably should have a flag to
      // turn it off just in case (e.g. case-sensitive filesystems).
      auto const& module_name = va_map.second.back()->module_->name_;
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
        HADESMEM_DETAIL_TRACE_NOISY_FORMAT_W(
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
            import_by_name_rva =
              descriptors_end_rva +
              static_cast<DWORD>(import_directories_buf.size()) -
              descriptors_size;

            // TODO: Set Hint.
            std::vector<char> import_by_name_buf(
              sizeof(IMAGE_IMPORT_BY_NAME::Hint) + e->name_.size() + 1);
            auto const import_by_name = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
              import_by_name_buf.data());
            std::copy(std::begin(e->name_),
                      std::end(e->name_),
                      &import_by_name->Name[0]);

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
    }

    std::size_t num_existing_imports = 0;
    (void)num_existing_imports;

    // TODO: Instead of adding the entries here unconditionally, we should
    // drop redundant entries.
    // TODO: Instead of just copying the entries over, we should use them as
    // the base of a scan earlier on, and just let them fall into our own
    // custom IAT naturally. This has the advantage of ensuring they are valid
    // and coalescing the location of all descriptors, at the slight
    // disadvantage of potentially missing some APIs (although that should be
    // quite rare as long as we implement the memory scan properly).
    for (auto const& dir : import_dirs)
    {
      HADESMEM_DETAIL_TRACE_A("Adding existing import directory.");

      hadesmem::ImportThunkList import_thunks_ft(
        local_process, pe_file_headers, dir.GetFirstThunk());
      auto const num_fts =
        std::distance(std::begin(import_thunks_ft), std::end(import_thunks_ft));
      hadesmem::ImportThunkList import_thunks_oft(
        local_process, pe_file_headers, dir.GetOriginalFirstThunk());
      auto const num_ofts = std::distance(std::begin(import_thunks_oft),
                                          std::end(import_thunks_oft));
      auto const num_new_existing_imports = (std::min)(num_fts, num_ofts);
      HADESMEM_DETAIL_TRACE_FORMAT_A("Adding %d existing imports.");
      num_existing_imports += num_new_existing_imports;

      auto const p = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(dir.GetBase());

      std::copy(reinterpret_cast<char const*>(p),
                reinterpret_cast<char const*>(p) + sizeof(*p),
                &import_directories_buf[descriptors_cur]);

      descriptors_cur += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }

    descriptors_cur += sizeof(IMAGE_IMPORT_DESCRIPTOR);

    HADESMEM_DETAIL_TRACE_FORMAT_A("Number of Scanned Imports: [%Iu].",
                                   num_imports);

    HADESMEM_DETAIL_TRACE_FORMAT_A("Number of Existing Imports: [%Iu].",
                                   num_existing_imports);

    // TODO: Should we just bail here if import_directories_buf is empty?

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

    HADESMEM_DETAIL_TRACE_A("Adding new import directories to file.");

    // Expand last section to its previous virtual end.
    raw_new.resize(last_section.GetPointerToRawData() + old_section_end);

    HADESMEM_DETAIL_ASSERT(raw_new.capacity() == raw_new_capacity);

    // Append our new data after the old virtual end.
    raw_new.reserve(raw_new.size() + (new_size - old_size));
    std::copy(std::begin(import_directories_buf),
              std::end(import_directories_buf),
              std::back_inserter(raw_new));

    HADESMEM_DETAIL_ASSERT(raw_new.capacity() == raw_new_capacity);

    // Expand to new size (because it's section aligned).
    auto const new_raw_size =
      last_section.GetPointerToRawData() + last_section.GetSizeOfRawData();
    raw_new.resize(new_raw_size);

    HADESMEM_DETAIL_ASSERT(raw_new.capacity() == raw_new_capacity);

    // TODO: Before we were fixing section sizes correctly above, the last
    // section data was truncated and we didn't appear to detect it properly
    // in the dump tool. Add support for this somewhere.

    HADESMEM_DETAIL_TRACE_A("Setting import data directory info.");

    nt_headers_new.SetDataDirectoryVirtualAddress(
      PeDataDir::Import, last_section.GetVirtualAddress() + old_section_end);
    nt_headers_new.SetDataDirectorySize(PeDataDir::Import, descriptors_cur);

    HADESMEM_DETAIL_TRACE_A("Setting image size.");

    auto const new_image_size = static_cast<DWORD>(
      RoundUp(last_section.GetVirtualAddress() + last_section.GetVirtualSize(),
              nt_headers.GetSectionAlignment()));
    nt_headers_new.SetSizeOfImage(new_image_size);

    // TODO: Fix checksums (with flag).

    // TODO: Strip DOS stub (with flag).

    nt_headers_new.UpdateWrite();
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Fixing imports.");

    ImportDirList const import_dirs(local_process, pe_file);
    ImportDirList const import_dirs_new(local_process, pe_file_new);
    auto i = std::begin(import_dirs), j = std::begin(import_dirs_new);
    bool thunk_mismatch = false;
    for (; i != std::end(import_dirs) && j != std::end(import_dirs_new);
         ++i, ++j)
    {
      // TODO: If we have an empty/invalid ILT we should enumerate the EAT of
      // all loaded modules and try to match exports to addresses in the IAT.
      // TODO: Add the option of using the on-disk ILT in the case we can't
      // find anything else?
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

    // TODO: In the case of an empty ILT we should only warn here instead of
    // erroring. We should also provide an option to use the on-disk IAT
    // (which acts as the ILT until the image is loaded) if it appears to
    // match, otherwise we will get dumps with a useless IAT for lots of
    // packed apps (e.g. Skyforge, Titanfall, Dragon Age: Inquisition, etc.).
    // TODO: Also update the implementation in CXExample.

    if (dir_mismatch)
    {
      HADESMEM_DETAIL_TRACE_A("Mismatch in import dir processing.");
    }

    if (thunk_mismatch)
    {
      HADESMEM_DETAIL_TRACE_A("Mismatch in import thunk processing.");
    }
  }

  // TODO: Write to a new sub-directory each time (e.g.
  // pe_dumps\foo.exe\1234\1).
  WriteDumpFile(process, m->name_, raw_new.data(), raw_new.size(), L"pe_dumps");
}

// TODO: Clean this up. It's a mess right now...
// TODO: Support running this on a memory dump (i.e. a module on disk, but in
// memory image form). Useful for reconstructing modules from crash dumps etc.
// TODO: Support 64-bit dumping 32-bit otherwise some targets will not be
// possible to dump due to memory pressure.
// TODO: Scan inside the region to uncover any modules which have been manual
// mapped not on a region boundary. For now lets start with scanning on a page
// bonadry. Will expand that later if required, although it will likely be a lot
// noisier so we may want to put it behind a flag.
// TODO: Add option to not dump system files?
// TODO: Split this into many smaller types and functions.
// TODO: Support plugins for import reconstruction.
// TODO: Disable relocations in dumped modules (just null out data directory
// entry, don't remove reloc section because it may contain more than just
// relocs).
// TODO: Rewrite direct imports. (i.e. Themida changes a jump through a function
// pointer into a direct jump, and inserts a nop to cover the extra byte. We
// need to be careful when rewriting to not assume that the nop is always before
// or after the jump, because it is seemingly randomly generated. So, instead of
// guessing and probably being wrong, we should write a jump to some dynamically
// generated code we insert into the PE which then in turn does the jump through
// the IAT. This way we both get our IAT reference and don't need to risk
// creating a bad dump. Another thing to consider is ensuring that the section
// we're inserting the code into is executable, and also whether we should add
// new sections instead of just appending to the last one like we have been so
// far. New sections would mean we could mark them properly, which could
// theoretically lead to better disassambly? It's hopeful thinking, but it can't
// hurt to try.)
// TODO: Don't trust values in the headers so much.
// TODO: Add optional OEP scan. Also make it optional whether to adjust the OEP
// in the headers, or simply add the OEP as a new custom export.
// TODO: Detect and resolve apphelp shims?
inline void DumpAllModules(Process const& process,
                           bool use_disk_headers,
                           bool reconstruct_imports,
                           bool add_new_section,
                           DWORD oep)
{
  auto const process_info = BuildExportMap(process);

  auto& modules = process_info.modules_;
  for (auto const& m : modules)
  {
    DumpSingleModule(process,
                     m.pe_file_.GetBase(),
                     use_disk_headers,
                     reconstruct_imports,
                     add_new_section,
                     oep,
                     &m);
  }
}

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
             bool use_disk_headers = false,
             bool reconstruct_imports = false,
             bool add_new_section = false,
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
    if (module_base)
    {
      DumpSingleModule(process,
                       base,
                       use_disk_headers,
                       reconstruct_imports,
                       add_new_section,
                       oep);
    }
    else
    {
      DumpAllModules(
        process, use_disk_headers, reconstruct_imports, add_new_section, oep);
    }
  }
}
}
}
