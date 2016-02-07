// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>

#include <psapi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/dos_header.hpp>
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

inline void WriteDumpFile(hadesmem::Process const& process,
                          std::wstring const& region_name,
                          void const* buffer,
                          std::size_t size,
                          std::wstring const& dir_name = L"dumps")
{
  HADESMEM_DETAIL_TRACE_A("Creating dump dir.");

  auto const dumps_dir =
    hadesmem::detail::CombinePath(hadesmem::detail::GetSelfDirPath(), dir_name);
  hadesmem::detail::CreateDirectoryWrapper(dumps_dir, false);

  HADESMEM_DETAIL_TRACE_A("Generating file name.");

  auto const proc_path = hadesmem::GetPath(process);
  auto const proc_name = proc_path.substr(proc_path.rfind(L'\\') + 1);
  auto const proc_pid_str = std::to_wstring(process.GetId());
  std::wstring dump_path;
  std::uint32_t c = 0;
  do
  {
    // TODO: Make a smarter extension selection (either use what the module
    // already has, or in the case of manually mapped modules we should try and
    // detect which extension to use based on the headers - for exe, dll, sys at
    // least).
    auto const file_name = proc_name + L"_" + proc_pid_str + L"_" +
                           region_name + L"_" + std::to_wstring(c++) + L".bin";
    dump_path = hadesmem::detail::CombinePath(dumps_dir, file_name);
  } while (hadesmem::detail::DoesFileExist(dump_path) && c < 10);

  if (c > 10)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error() << hadesmem::ErrorString(
        "Found more than 10 conflicting file names. Aborting."));
  }

  HADESMEM_DETAIL_TRACE_A("Opening file.");

  auto const dump_file = hadesmem::detail::OpenFile<char>(
    dump_path, std::ios::out | std::ios::binary);
  if (!*dump_file)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error() << hadesmem::ErrorString("Unable to open dump file."));
  }

  HADESMEM_DETAIL_TRACE_A("Writing file.");

  if (!dump_file->write(static_cast<char const*>(buffer), size))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error() << hadesmem::ErrorString(
                                      "Unable to write to dump file."));
  }
}

inline void DumpAllModules(hadesmem::Process const& process,
                           bool use_disk_headers)
{
  HADESMEM_DETAIL_TRACE_A("Starting module enumeration.");

  hadesmem::ModuleList modules(process);
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
    auto const module_size =
      hadesmem::detail::GetRegionAllocSize(process, module.GetHandle());
    if (module_size > static_cast<DWORD>(-1))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error() << hadesmem::ErrorString("Region too large."));
    }

    HADESMEM_DETAIL_TRACE_FORMAT_A("Calculated module size: [%08lX].",
                                   static_cast<DWORD>(module_size));

    try
    {
      hadesmem::PeFile const pe_file(process,
                                     module.GetHandle(),
                                     hadesmem::PeFileType::Image,
                                     static_cast<DWORD>(module_size));
      hadesmem::NtHeaders nt_headers(process, pe_file);
    }
    catch (std::exception const& /*e*/)
    {
      HADESMEM_DETAIL_TRACE_A("WARNING! Invalid headers.");
      return;
    }

    HADESMEM_DETAIL_TRACE_A("Reading memory.");

    auto raw = hadesmem::ReadVectorEx<std::uint8_t>(
      process,
      module.GetHandle(),
      module_size,
      hadesmem::ReadFlags::kZeroFillReserved);
    hadesmem::Process const local_process(::GetCurrentProcessId());
    hadesmem::PeFile const pe_file(local_process,
                                   raw.data(),
                                   hadesmem::PeFileType::Image,
                                   static_cast<DWORD>(raw.size()));

    std::vector<char> pe_file_disk_data;
    std::unique_ptr<hadesmem::PeFile> pe_file_disk;
    if (use_disk_headers)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Using disk headers. Path: [%s].",
                                     module.GetPath().c_str());

      pe_file_disk_data = hadesmem::detail::PeFileToBuffer(module.GetPath());
      pe_file_disk =
        std::make_unique<hadesmem::PeFile>(local_process,
                                           pe_file_disk_data.data(),
                                           hadesmem::PeFileType::Data,
                                           pe_file_disk_data.size());
    }

    hadesmem::PeFile const& pe_file_headers =
      use_disk_headers ? *pe_file_disk : pe_file;
    hadesmem::NtHeaders nt_headers(local_process, pe_file_headers);

    HADESMEM_DETAIL_TRACE_A("Copying headers.");

    std::vector<std::uint8_t> raw_new(nt_headers.GetSizeOfHeaders());
    auto const headers_buf_beg =
      use_disk_headers
        ? reinterpret_cast<std::uint8_t const*>(pe_file_disk_data.data())
        : raw.data();
    std::copy(headers_buf_beg,
              headers_buf_beg + nt_headers.GetSizeOfHeaders(),
              raw_new.data());

    HADESMEM_DETAIL_TRACE_A("Copying section data.");

    hadesmem::SectionList const sections(local_process, pe_file_headers);
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

      auto const section_size =
        (std::max)(section.GetVirtualSize(), section.GetSizeOfRawData());
      auto const ptr_raw_data_new =
        section.GetPointerToRawData() < raw_new.size()
          ? static_cast<DWORD>(
              RoundUp(raw_new.size(), nt_headers.GetFileAlignment()))
          : section.GetPointerToRawData();
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
    hadesmem::PeFile const pe_file_new(local_process,
                                       raw_new.data(),
                                       hadesmem::PeFileType::Data,
                                       static_cast<DWORD>(raw_new.size()));

    HADESMEM_DETAIL_TRACE_A("Fixing NT headers.");

    hadesmem::NtHeaders nt_headers_new(local_process, pe_file_new);
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
      hadesmem::NtHeaders nt_headers_disk(local_process, *pe_file_disk);
      HADESMEM_DETAIL_TRACE_FORMAT_A("AddressOfEntryPoint: [%08lX] -> [%08lX].",
                                     nt_headers_new.GetAddressOfEntryPoint(),
                                     nt_headers_disk.GetAddressOfEntryPoint());
      nt_headers_new.SetAddressOfEntryPoint(
        nt_headers_disk.GetAddressOfEntryPoint());
    }
    nt_headers_new.UpdateWrite();

    HADESMEM_DETAIL_TRACE_A("Fixing section headers.");

    hadesmem::SectionList sections_new(local_process, pe_file_new);
    std::size_t n = 0;
    for (auto& section : sections_new)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A("PointerToRawData: [%08lX] -> [%08lX]. "
                                     "SizeOfRawData: [%08lX] -> [%08lX].",
                                     section.GetPointerToRawData(),
                                     raw_datas[n].first,
                                     section.GetSizeOfRawData(),
                                     raw_datas[n].second);

      section.SetPointerToRawData(raw_datas[n].first);
      section.SetSizeOfRawData(raw_datas[n].second);
      section.UpdateWrite();
      ++n;
    }

    HADESMEM_DETAIL_TRACE_A("Fixing imports.");

    hadesmem::ImportDirList const import_dirs(local_process, pe_file);
    hadesmem::ImportDirList const import_dirs_new(local_process, pe_file_new);
    auto i = std::begin(import_dirs), j = std::begin(import_dirs_new);
    bool thunk_mismatch = false;
    for (; i != std::end(import_dirs) && j != std::end(import_dirs_new);
         ++i, ++j)
    {
      // TODO: If we have an empty/invalid ILT we should enumerate the EAT of
      // all loaded modules and try to match exports to addresses in the IAT.
      // TODO: Add the option of using the on-disk ILT in the case we can't find
      // anything else?
      hadesmem::ImportThunkList const import_thunks(
        local_process, pe_file, i->GetOriginalFirstThunk());
      hadesmem::ImportThunkList import_thunks_new(
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

    WriteDumpFile(
      process, module.GetName(), raw_new.data(), raw_new.size(), L"pe_dumps");

    // TODO: In the case of an empty ILT we should only warn here instead of
    // erroring. We should also provide an option to use the on-disk IAT (which
    // acts as the ILT until the image is loaded) if it appears to match,
    // otherwise we will get dumps with a useless IAT for lots of packed apps
    // (e.g. Skyforge, Titanfall, Dragon Age: Inquisition, etc.).
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
}

inline void DumpMemoryRegionRaw(hadesmem::Process const& process,
                                void* base,
                                std::size_t size)
{
  HADESMEM_DETAIL_TRACE_A("Reading memory.");

  auto raw = hadesmem::ReadVector<std::uint8_t>(process, base, size);

  WriteDumpFile(
    process, PtrToHexString(base), raw.data(), raw.size(), L"raw_dumps");
}

inline void DumpMemory(
  hadesmem::Process const& process = hadesmem::Process(::GetCurrentProcessId()),
  bool use_disk_headers = false,
  void* base = nullptr,
  std::size_t size = 0)
{
  if (!base && !size)
  {
    DumpAllModules(process, use_disk_headers);
  }
  else
  {
    // TODO: Actually implement support for utilizing this in Dump tool.
    DumpMemoryRegionRaw(process, base, size);
  }
}
}
}
