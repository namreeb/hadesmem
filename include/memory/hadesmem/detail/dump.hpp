// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>

#include <psapi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/module_list.hpp>
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

// TODO: Clean this up. It's a mess right now...
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

    ModuleList modules(process);
    for (auto const& module : modules)
    {
      // We can't use the module size returned by Module32First/Module32Next
      // because it can be too large.
      auto const module_size = GetRegionAllocSize(process, module.GetHandle());
      if (module_size > static_cast<DWORD>(-1))
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                        << ErrorString("Region too large."));
      }

      std::unique_ptr<PeFile> pe_file;
      std::unique_ptr<NtHeaders> nt_headers;
      try
      {
        pe_file = std::make_unique<PeFile>(process,
                                           module.GetHandle(),
                                           PeFileType::Image,
                                           static_cast<DWORD>(module_size));
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
          reconstruct_info.module_name_ = WideCharToMultiByte(module.GetName());
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

  ModuleList modules(process);
  for (auto const& module : modules)
  {
    HADESMEM_DETAIL_TRACE_A("Got new module.");

    HADESMEM_DETAIL_TRACE_FORMAT_W(
      L"Handle: [%p]. Size: [%08lX]. Name: [%s]. Path: [%s].",
      module.GetHandle(),
      module.GetSize(),
      module.GetName().c_str(),
      module.GetPath().c_str());

    HADESMEM_DETAIL_TRACE_A("Checking for valid headers.");

    // We can't use the module size returned by Module32First/Module32Next
    // because it can be too large.
    auto const module_size = GetRegionAllocSize(process, module.GetHandle());
    if (module_size > static_cast<DWORD>(-1))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Region too large."));
    }

    HADESMEM_DETAIL_TRACE_FORMAT_A("Calculated module size: [%08lX].",
                                   static_cast<DWORD>(module_size));

    try
    {
      PeFile const pe_file(process,
                           module.GetHandle(),
                           PeFileType::Image,
                           static_cast<DWORD>(module_size));
      NtHeaders nt_headers(process, pe_file);
    }
    catch (std::exception const& /*e*/)
    {
      HADESMEM_DETAIL_TRACE_A("WARNING! Invalid headers.");
      continue;
    }

    HADESMEM_DETAIL_TRACE_A("Reading memory.");

    auto raw = ReadVectorEx<std::uint8_t>(
      process, module.GetHandle(), module_size, ReadFlags::kZeroFillReserved);
    Process const local_process(::GetCurrentProcessId());
    PeFile const pe_file(local_process,
                         raw.data(),
                         PeFileType::Image,
                         static_cast<DWORD>(raw.size()));

    std::vector<char> pe_file_disk_data;
    std::unique_ptr<PeFile> pe_file_disk;
    if (use_disk_headers)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Using disk headers. Path: [%s].",
                                     module.GetPath().c_str());

      // TODO: Use GetMappedFileName instead so we can also support unlinked
      // modules.
      // TODO: Support not failing here if we don't have file headers, because
      // we want to support dumping manually mapped PE files with headers.
      // (Sounds like it defeats the purpose but it's more common than you might
      // think...)
      pe_file_disk_data = PeFileToBuffer(module.GetPath());
      pe_file_disk =
        std::make_unique<PeFile>(local_process,
                                 pe_file_disk_data.data(),
                                 PeFileType::Data,
                                 static_cast<DWORD>(pe_file_disk_data.size()));
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

      auto const raw_data = raw.data() + section.GetVirtualAddress();
      auto const raw_data_end = raw_data + section_size;
      raw_new.reserve(raw_new.size() + section_size);
      std::copy(raw_data, raw_data_end, std::back_inserter(raw_new));
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
      static_cast<ULONGLONG>(reinterpret_cast<ULONG_PTR>(module.GetHandle()));
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
      // TODO: Manually insert the on-disk and/or in-memory entries at the
      // start (and ensure we don't double them up during our scan).
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

        // Only fixup page aligned VAs if they are adjacent to another fixup,
        // because it would be too risky to reconstruct them otherwise (high
        // chance of false positives when scanning for references).
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

      // Build raw import dirs.
      std::vector<char> import_directories_buf;
      ImportDirList import_dirs(local_process, pe_file);
      auto const descriptors_size = static_cast<DWORD>(
        sizeof(IMAGE_IMPORT_DESCRIPTOR) *
        (std::distance(std::begin(import_dirs), std::end(import_dirs)) +
         va_reconstruct_list.size() + 1));
      auto const descriptors_end_rva =
        last_section.GetVirtualAddress() + old_section_end + descriptors_size;
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
      for (auto const& v : va_reconstruct_list)
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

        // TODO: Don't require one descriptor per thunk if they're all adjacent
        // and in the same module.

        // TODO: Reuse by name data for thunks with the same name.

        DWORD const module_name_rva =
          descriptors_end_rva +
          static_cast<DWORD>(import_directories_buf.size()) - descriptors_size;
        std::copy(std::begin(v.module_name_),
                  std::end(v.module_name_),
                  std::back_inserter(import_directories_buf));
        import_directories_buf.push_back('\0');

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
          auto const import_by_name =
            reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(import_by_name_buf.data());
          std::copy(
            std::begin(v.name_), std::end(v.name_), &import_by_name->Name[0]);

          std::copy(std::begin(import_by_name_buf),
                    std::end(import_by_name_buf),
                    std::back_inserter(import_directories_buf));
        }

        DWORD const thunk_rva =
          descriptors_end_rva +
          static_cast<DWORD>(import_directories_buf.size()) - descriptors_size;
        IMAGE_THUNK_DATA thunk{};
        if (v.by_name_)
        {
          thunk.u1.AddressOfData = import_by_name_rva;
        }
        else
        {
          thunk.u1.Ordinal =
            IMAGE_ORDINAL_FLAG | (v.ordinal_ & static_cast<WORD>(-1));
        }
        std::copy(reinterpret_cast<char const*>(&thunk),
                  reinterpret_cast<char const*>(&thunk) + sizeof(thunk),
                  std::back_inserter(import_directories_buf));
        IMAGE_THUNK_DATA empty_thunk{};
        std::copy(reinterpret_cast<char const*>(&empty_thunk),
                  reinterpret_cast<char const*>(&empty_thunk) +
                    sizeof(empty_thunk),
                  std::back_inserter(import_directories_buf));

        IMAGE_IMPORT_DESCRIPTOR descriptor{};
        descriptor.OriginalFirstThunk = thunk_rva;
        descriptor.TimeDateStamp = static_cast<DWORD>(-1);
        descriptor.ForwarderChain = static_cast<DWORD>(-1);
        descriptor.Name = module_name_rva;
        descriptor.FirstThunk = v.rva_;
        std::copy(reinterpret_cast<char const*>(&descriptor),
                  reinterpret_cast<char const*>(&descriptor) +
                    sizeof(descriptor),
                  &import_directories_buf[descriptors_cur]);

        descriptors_cur += sizeof(IMAGE_IMPORT_DESCRIPTOR);
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
      process, module.GetName(), raw_new.data(), raw_new.size(), L"pe_dumps");
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
