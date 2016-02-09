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

inline std::wstring GetRegionName(Process const& process, void* p)
{
  std::vector<wchar_t> mapped_file_name(HADESMEM_DETAIL_MAX_PATH_UNICODE);
  if (::GetMappedFileNameW(process.GetHandle(),
                           p,
                           mapped_file_name.data(),
                           static_cast<DWORD>(mapped_file_name.size())))
  {
    auto const region_name = static_cast<std::wstring>(mapped_file_name.data());
    return region_name.substr(region_name.rfind(L'\\') + 1);
  }

  DWORD const last_error = ::GetLastError();
  HADESMEM_DETAIL_TRACE_FORMAT_A("WARNING! GetMappedFileNameW failed. "
                                 "Defaulting to base address as string. "
                                 "LastError: [%08lX].",
                                 last_error);

  return L"unknown_" + PtrToHexString(p);
}

inline std::wstring GetRegionPath(Process const& process, void* p)
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

// TODO: Clean this up. It's a mess right now...
// TODO: Support running this on a memory dump (i.e. a module on disk, but in
// memory image form). Useful for reconstructing modules from crash dumps etc.
inline void DumpAllModules(Process const& process,
                           bool use_disk_headers,
                           bool reconstruct_imports)
{
  struct VaReconstructInfo
  {
    // RVA of the reference to the export. i.e. The RVA of the fixup.
    DWORD rva_{};

    // VA of the export (not the reference!).
    void* va_{};
    std::string module_name_;
    bool by_name_{};
    std::string name_;
    DWORD ordinal_{};
  };

  // TODO: Fix code duplication between this branch and the rest of the function
  // (getting module size, checking if PE file is valid, etc.).
  std::map<void*, VaReconstructInfo> va_reconstruct_map;
  if (reconstruct_imports)
  {
    HADESMEM_DETAIL_TRACE_A("Building export list.");

    RegionList regions(process);
    for (auto const& region : regions)
    {
      auto const base = region.GetBase();
      if (!base)
      {
        continue;
      }

      // TODO: Verify the pages.

      // TODO: Better validations.
      if (!CanRead(process, base) || IsBadProtect(process, base))
      {
        continue;
      }

      auto const mz = Read<std::array<char, 2>>(process, base);
      if (mz[0] != 'M' || mz[1] != 'Z')
      {
        continue;
      }

      // We can't use the module size returned by Module32First/Module32Next
      // because it can be too large.
      auto const module_size = GetRegionAllocSize(process, base, true);
      if (module_size > static_cast<DWORD>(-1))
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                        << ErrorString("Region too large."));
      }

      std::unique_ptr<PeFile> pe_file;
      std::unique_ptr<NtHeaders> nt_headers;
      try
      {
        pe_file = std::make_unique<PeFile>(
          process, base, PeFileType::Image, static_cast<DWORD>(module_size));
        nt_headers = std::make_unique<NtHeaders>(process, *pe_file);
      }
      catch (std::exception const& /*e*/)
      {
        HADESMEM_DETAIL_TRACE_A("WARNING! Invalid headers.");
        return;
      }

      ExportList exports(process, *pe_file);
      for (auto const& e : exports)
      {
        if (e.IsVirtualVa())
        {
          HADESMEM_DETAIL_TRACE_A("WARNING! Skipping virtual VA.");
          continue;
        }

        if (!e.GetVa())
        {
          // Don't warn on forwarded functions with a zero VA because it would
          // be too noisy otherwise.
          if (!e.IsForwarded())
          {
            HADESMEM_DETAIL_TRACE_A("WARNING! Skipping zero VA.");
          }
          continue;
        }

        if (!va_reconstruct_map.count(e.GetVa()))
        {
          VaReconstructInfo reconstruct_info;
          // TODO: What about the module name in the export directory?
          reconstruct_info.va_ = e.GetVa();
          reconstruct_info.module_name_ =
            WideCharToMultiByte(GetRegionName(process, base));
          reconstruct_info.by_name_ = e.ByName();
          if (reconstruct_info.by_name_)
          {
            reconstruct_info.name_ = e.GetName();
          }
          else
          {
            reconstruct_info.ordinal_ = e.GetProcedureNumber();
          }

          va_reconstruct_map[e.GetVa()] = reconstruct_info;
        }
      }
    }
  }

  HADESMEM_DETAIL_TRACE_A("Starting module enumeration.");

  // TODO: Reduce code duplication between this and the above loop.

  RegionList regions(process);
  for (auto const& region : regions)
  {
    auto const base = region.GetBase();
    if (!base)
    {
      continue;
    }

    // TODO: Verify the pages.

    // TODO: Better validations.
    if (!CanRead(process, base) || IsBadProtect(process, base))
    {
      continue;
    }

    auto const mz = Read<std::array<char, 2>>(process, base);
    if (mz[0] != 'M' || mz[1] != 'Z')
    {
      continue;
    }

    HADESMEM_DETAIL_TRACE_A("Got new target region.");

    HADESMEM_DETAIL_TRACE_FORMAT_W(
      L"Base: [%p]. Size: [%08lX]. Protect: [%08lX]. Type: [%08lX].",
      region.GetBase(),
      region.GetSize(),
      region.GetProtect(),
      region.GetType());

    HADESMEM_DETAIL_TRACE_A("Checking for valid headers.");

    // We can't use the module size returned by Module32First/Module32Next
    // because it can be too large.
    auto const module_size = GetRegionAllocSize(process, base);
    if (module_size > static_cast<DWORD>(-1))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Region too large."));
    }

    HADESMEM_DETAIL_TRACE_FORMAT_A("Calculated module size: [%08lX].",
                                   static_cast<DWORD>(module_size));

    try
    {
      PeFile const pe_file(
        process, base, PeFileType::Image, static_cast<DWORD>(module_size));
      NtHeaders nt_headers(process, pe_file);
    }
    catch (std::exception const& /*e*/)
    {
      HADESMEM_DETAIL_TRACE_A("WARNING! Invalid headers.");
      continue;
    }

    HADESMEM_DETAIL_TRACE_A("Reading memory.");

    auto raw = ReadVectorEx<std::uint8_t>(
      process, region.GetBase(), module_size, ReadFlags::kZeroFillReserved);
    Process const local_process(::GetCurrentProcessId());
    PeFile const pe_file(local_process,
                         raw.data(),
                         PeFileType::Image,
                         static_cast<DWORD>(raw.size()));

    auto const region_name = GetRegionName(process, base);
    auto const region_path = GetRegionPath(process, base);

    if (region_path.empty())
    {
      HADESMEM_DETAIL_TRACE_A(
        "WARNING! No region path, not attempting to use disk headers.");
      use_disk_headers = false;
    }

    // TODO: Don't hard-fail if we request disk headers but can't get them?

    std::vector<char> pe_file_disk_data;
    std::unique_ptr<PeFile> pe_file_disk;
    if (use_disk_headers)
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
        pe_file_disk = std::make_unique<PeFile>(
          local_process,
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

        use_disk_headers = false;
      }
    }

    PeFile const& pe_file_headers = use_disk_headers ? *pe_file_disk : pe_file;
    NtHeaders nt_headers(local_process, pe_file_headers);

    HADESMEM_DETAIL_TRACE_A("Copying headers.");

    std::vector<std::uint8_t> raw_new(nt_headers.GetSizeOfHeaders());
    // TODO: Fix this hack properly. We reserve plenty of size in the buffer so
    // when we open the PE file object later pointing to the buffer, if we add
    // more data to the buffer it won't be reallocated.
    raw_new.reserve(module_size * 2);
    auto const headers_buf_beg =
      use_disk_headers
        ? reinterpret_cast<std::uint8_t const*>(pe_file_disk_data.data())
        : raw.data();
    std::copy(headers_buf_beg,
              headers_buf_beg + nt_headers.GetSizeOfHeaders(),
              raw_new.data());

    HADESMEM_DETAIL_TRACE_A("Copying section data.");

    SectionList const sections(local_process, pe_file_headers);
    std::vector<std::pair<DWORD, DWORD>> raw_datas;
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
      // zero-fill pages at the end and then drop as many as possible in order
      // to reduce file size.
      // TODO: Validate section virtual sizes against the VA region sizes?
      // TODO: Automatically align sections? (e.g. Bump section virtual size
      // from 0x400 to 0x1000).

      auto const section_size = static_cast<DWORD>(RoundUp(
        (std::max)(section.GetVirtualSize(), section.GetSizeOfRawData()),
        nt_headers.GetSectionAlignment()));
      auto const ptr_raw_data_new_tmp =
        section.GetPointerToRawData() < raw_new.size()
          ? static_cast<DWORD>(
              RoundUp(raw_new.size(), nt_headers.GetFileAlignment()))
          : section.GetPointerToRawData();
      auto const ptr_raw_data_new = static_cast<DWORD>(
        RoundUp(ptr_raw_data_new_tmp, nt_headers.GetSectionAlignment()));
      raw_datas.emplace_back(ptr_raw_data_new, section_size);

      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "New PointerToRawData: [%08lX]. New SizeOfRawData: [%08lX].",
        ptr_raw_data_new,
        section_size);

      if (ptr_raw_data_new > raw_new.size())
      {
        raw_new.resize(ptr_raw_data_new);
      }

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
    }

    HADESMEM_DETAIL_ASSERT(raw_new.size() <
                           (std::numeric_limits<DWORD>::max)());
    PeFile const pe_file_new(local_process,
                             raw_new.data(),
                             PeFileType::Data,
                             static_cast<DWORD>(raw_new.size()));

    HADESMEM_DETAIL_TRACE_A("Fixing NT headers.");

    NtHeaders nt_headers_new(local_process, pe_file_new);
    auto const image_base_new =
      static_cast<ULONGLONG>(reinterpret_cast<ULONG_PTR>(base));
    HADESMEM_DETAIL_TRACE_FORMAT_A("ImageBase: [%016llX] -> [%016llX].",
                                   nt_headers_new.GetImageBase(),
                                   image_base_new);
    nt_headers_new.SetImageBase(image_base_new);
    // TODO: Attempt this even in the case that the use hasn't requested use of
    // disk headers, because something is better than nothing. Just make sure if
    // we can't load the file that we fail gracefully and continue.
    if (use_disk_headers && !nt_headers_new.GetAddressOfEntryPoint())
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
                                     raw_datas[n].first,
                                     section.GetSizeOfRawData(),
                                     raw_datas[n].second,
                                     section.GetVirtualSize(),
                                     raw_datas[n].second);

      section.SetPointerToRawData(raw_datas[n].first);
      section.SetSizeOfRawData(raw_datas[n].second);
      section.SetVirtualSize(raw_datas[n].second);
      section.UpdateWrite();
      ++n;
    }

    // Expand the last section to hold our new import directories.
    // TODO: Add a new section instead of expanding the last section.
    if (reconstruct_imports)
    {
      HADESMEM_DETAIL_TRACE_A("Reconstructing imports.");

      std::vector<Section> sections_vec;
      std::copy(std::begin(sections_new),
                std::end(sections_new),
                std::back_inserter(sections_vec));

      HADESMEM_DETAIL_TRACE_A("Getting last section info.");

      auto& last_section = sections_vec[sections_vec.size() - 1];
      auto const old_size = last_section.GetSizeOfRawData();
      auto const old_section_end = static_cast<DWORD>(RoundUp(
        last_section.GetVirtualSize(), nt_headers_new.GetSectionAlignment()));

      HADESMEM_DETAIL_TRACE_A("Performing memory scan.");

      // Get list of required fixups.
      std::vector<VaReconstructInfo> va_reconstruct_list;
      bool fixup_adjacent = false;
      for (auto p = raw_new.data(); p < raw_new.data() + raw_new.size() - 3;
           p += 4)
      {
        auto const va = *reinterpret_cast<void**>(p);
        auto const e = va_reconstruct_map.find(va);
        if (e == std::end(va_reconstruct_map))
        {
          fixup_adjacent = false;
          continue;
        }

        auto v = e->second;
        HADESMEM_DETAIL_ASSERT(v.va_ == va);
        HADESMEM_DETAIL_TRACE_FORMAT_A("Found matching VA. VA: [%p]. Module: "
                                       "[%s]. Name: [%s]. Ordinal: [%lu]. "
                                       "ByName: [%d].",
                                       v.va_,
                                       v.module_name_.c_str(),
                                       v.name_.c_str(),
                                       v.ordinal_,
                                       v.by_name_);

        auto const offset =
          static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(p) -
                             reinterpret_cast<std::uintptr_t>(raw_new.data()));
        v.rva_ = FileOffsetToRva(
          local_process, pe_file_new, static_cast<DWORD>(offset));

        // TODO: Use module base/size instead of name.
        if (v.module_name_ ==
            hadesmem::detail::WideCharToMultiByte(region_name))
        {
          HADESMEM_DETAIL_TRACE_FORMAT_A(
            "WARNING! Skipping VA in current module. "
            "VA: [%p]. Offset: [%08lX]. RVA: [%08lX].",
            e->first,
            offset,
            v.rva_);
          continue;
        }

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
          auto const next_e = va_reconstruct_map.find(next_va);
          if (next_e == std::end(va_reconstruct_map))
          {
            HADESMEM_DETAIL_TRACE_FORMAT_A("WARNING! Skipping page aligned VA. "
                                           "VA: [%p]. Offset: [%08lX]. RVA: "
                                           "[%08lX].",
                                           e->first,
                                           offset,
                                           v.rva_);
            continue;
          }
        }

        HADESMEM_DETAIL_TRACE_FORMAT_A(
          "Adding match to fixup list. Offset: [%08lX]. RVA: [%08lX].",
          offset,
          v.rva_);

        va_reconstruct_list.push_back(v);
        fixup_adjacent = true;
        p += sizeof(void*) - 4;
      }

      if (va_reconstruct_list.empty())
      {
        HADESMEM_DETAIL_TRACE_A("WARNING! Empty import reconstruction list.");
      }

      HADESMEM_DETAIL_TRACE_A("Building raw import directories buffer.");

      // TODO: Don't require so many transformatins. This could be made far
      // cleaner and more efficient.

      // Remove all references to the module currently being dumped
      // TODO: Use module base/size instead of name.
      va_reconstruct_list.erase(
        std::remove_if(std::begin(va_reconstruct_list),
                       std::end(va_reconstruct_list),
                       [&](VaReconstructInfo const& v)
                       {
                         return v.module_name_ ==
                                hadesmem::detail::WideCharToMultiByte(
                                  region_name);
                       }),
        std::end(va_reconstruct_list));

      // Transform import reconstruction list into one grouped by module name.
      std::map<DWORD, std::vector<VaReconstructInfo>> va_reconstruct_info_by_ft;
      DWORD cur_rva_base = 0;
      DWORD prev_rva = 0;
      for (auto const& v : va_reconstruct_list)
      {
        if (!prev_rva || v.rva_ != prev_rva + sizeof(void*))
        {
          cur_rva_base = v.rva_;
        }

        va_reconstruct_info_by_ft[cur_rva_base].emplace_back(v);

        prev_rva = v.rva_;
      }

      // Build raw import dirs.
      std::vector<char> import_directories_buf;
      ImportDirList import_dirs(local_process, pe_file_headers);
      auto const descriptors_size = static_cast<DWORD>(
        sizeof(IMAGE_IMPORT_DESCRIPTOR) *
        (std::distance(std::begin(import_dirs), std::end(import_dirs)) +
         va_reconstruct_info_by_ft.size() + 1));
      auto const descriptors_end_rva =
        last_section.GetVirtualAddress() + old_section_end + descriptors_size;
      // Fix this hack (see above for the same thing being done to raw_new).
      import_directories_buf.reserve(module_size * 2);
      import_directories_buf.resize(descriptors_size);
      DWORD descriptors_cur = 0;

      for (auto const& dir : import_dirs)
      {
        HADESMEM_DETAIL_TRACE_A("Adding existing import directory.");

        auto const p =
          reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(dir.GetBase());

        std::copy(reinterpret_cast<char const*>(p),
                  reinterpret_cast<char const*>(p) + sizeof(*p),
                  &import_directories_buf[descriptors_cur]);

        descriptors_cur += sizeof(IMAGE_IMPORT_DESCRIPTOR);
      }

      // TODO: Don't duplicate entries from existing IAT (above).
      for (auto const& va_map : va_reconstruct_info_by_ft)
      {
        // TODO: Reuse this where possible.
        DWORD const module_name_rva =
          descriptors_end_rva +
          static_cast<DWORD>(import_directories_buf.size()) - descriptors_size;
        auto const module_name = va_map.second.back().module_name_;
        std::copy(std::begin(module_name),
                  std::end(module_name),
                  std::back_inserter(import_directories_buf));
        import_directories_buf.push_back('\0');

        DWORD const thunks_rva =
          descriptors_end_rva +
          static_cast<DWORD>(import_directories_buf.size()) - descriptors_size;
        auto const thunks_offset = import_directories_buf.size();
        import_directories_buf.resize(thunks_offset +
                                      sizeof(IMAGE_THUNK_DATA) *
                                        (va_map.second.size() + 1));
        auto thunks = reinterpret_cast<PIMAGE_THUNK_DATA>(
          &import_directories_buf[thunks_offset]);

        IMAGE_IMPORT_DESCRIPTOR descriptor{};
        descriptor.OriginalFirstThunk = thunks_rva;
        descriptor.TimeDateStamp = static_cast<DWORD>(-1);
        descriptor.ForwarderChain = static_cast<DWORD>(-1);
        descriptor.Name = module_name_rva;
        descriptor.FirstThunk = va_map.first;
        std::copy(reinterpret_cast<char const*>(&descriptor),
                  reinterpret_cast<char const*>(&descriptor) +
                    sizeof(descriptor),
                  &import_directories_buf[descriptors_cur]);

        descriptors_cur += sizeof(IMAGE_IMPORT_DESCRIPTOR);

        for (auto const& v : va_map.second)
        {
          HADESMEM_DETAIL_TRACE_FORMAT_A(
            "Processing import reconstruction entry. RVA: [%08lX], VA: "
            "[%p]. Module: [%s]. Name: [%s]. Ordinal: "
            "[%lu]. ByName: [%d].",
            v.rva_,
            v.va_,
            v.module_name_.c_str(),
            v.name_.c_str(),
            v.ordinal_,
            v.by_name_);

          // TODO: Reuse by name data for thunks with the same name.

          DWORD const import_by_name_rva =
            v.by_name_
              ? descriptors_end_rva +
                  static_cast<DWORD>(import_directories_buf.size()) -
                  descriptors_size
              : 0;
          if (v.by_name_)
          {
            std::vector<char> import_by_name_buf(
              sizeof(IMAGE_IMPORT_BY_NAME::Hint) + v.name_.size() + 1);
            auto const import_by_name = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
              import_by_name_buf.data());
            std::copy(
              std::begin(v.name_), std::end(v.name_), &import_by_name->Name[0]);

            std::copy(std::begin(import_by_name_buf),
                      std::end(import_by_name_buf),
                      std::back_inserter(import_directories_buf));
          }

          if (v.by_name_)
          {
            thunks->u1.AddressOfData = import_by_name_rva;
          }
          else
          {
            thunks->u1.Ordinal =
              IMAGE_ORDINAL_FLAG | (v.ordinal_ & static_cast<WORD>(-1));
          }

          ++thunks;
        }
      }

      descriptors_cur += sizeof(IMAGE_IMPORT_DESCRIPTOR);

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

      // Append our new data after the old virtual end.
      raw_new.reserve(raw_new.size() + (new_size - old_size));
      std::copy(std::begin(import_directories_buf),
                std::end(import_directories_buf),
                std::back_inserter(raw_new));

      // Expand to new size (because it's section aligned).
      auto const new_raw_size =
        last_section.GetPointerToRawData() + last_section.GetSizeOfRawData();
      raw_new.resize(new_raw_size);

      // TODO: Before we were fixing section sizes correctly above, the last
      // section data was truncated and we didn't appear to detect it properly
      // in the dump tool. Add support for this somewhere.

      HADESMEM_DETAIL_TRACE_A("Setting import data directory info.");

      nt_headers_new.SetDataDirectoryVirtualAddress(
        PeDataDir::Import, last_section.GetVirtualAddress() + old_section_end);
      nt_headers_new.SetDataDirectorySize(PeDataDir::Import, descriptors_cur);

      HADESMEM_DETAIL_TRACE_A("Setting image size.");

      auto const new_image_size = static_cast<DWORD>(RoundUp(
        last_section.GetVirtualAddress() + last_section.GetVirtualSize(),
        nt_headers.GetSectionAlignment()));
      nt_headers_new.SetSizeOfImage(new_image_size);

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

    WriteDumpFile(
      process, region_name, raw_new.data(), raw_new.size(), L"pe_dumps");
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
             void* base = nullptr,
             std::size_t size = 0)
{
  if (!base && !size)
  {
    DumpAllModules(process, use_disk_headers, reconstruct_imports);
  }
  else
  {
    // TODO: Actually implement support for utilizing this in Dump tool.
    DumpMemoryRegionRaw(process, base, size);
  }
}
}
}
