// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>

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
  HADESMEM_DETAIL_TRACE_A("Writing file.");

  auto const proc_path = hadesmem::GetPath(process);
  auto const proc_name = proc_path.substr(proc_path.rfind(L'\\') + 1);
  auto const proc_pid_str = std::to_wstring(process.GetId());
  auto const dumps_dir =
    hadesmem::detail::CombinePath(hadesmem::detail::GetSelfDirPath(), dir_name);
  hadesmem::detail::CreateDirectoryWrapper(dumps_dir, false);
  std::wstring dump_path;
  std::uint32_t c = 0;
  do
  {
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

  auto const dump_file = hadesmem::detail::OpenFile<char>(
    dump_path, std::ios::out | std::ios::binary);
  if (!*dump_file)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error() << hadesmem::ErrorString("Unable to open dump file."));
  }

  if (!dump_file->write(static_cast<char const*>(buffer), size))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error() << hadesmem::ErrorString(
                                      "Unable to write to dump file."));
  }
}

inline void DumpAllModules(hadesmem::Process const& process)
{
  HADESMEM_DETAIL_TRACE_A("Starting module enumeration.");

  hadesmem::ModuleList modules(process);
  for (auto const& module : modules)
  {
    HADESMEM_DETAIL_TRACE_A("Checking for valid headers.");

    try
    {
      hadesmem::PeFile const pe_file(process,
                                     module.GetHandle(),
                                     hadesmem::PeFileType::Image,
                                     static_cast<DWORD>(module.GetSize()));
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
      module.GetSize(),
      hadesmem::ReadFlags::kZeroFillReserved);
    hadesmem::Process const local_process(::GetCurrentProcessId());
    hadesmem::PeFile const pe_file(local_process,
                                   raw.data(),
                                   hadesmem::PeFileType::Image,
                                   static_cast<DWORD>(raw.size()));
    hadesmem::NtHeaders nt_headers(local_process, pe_file);

    HADESMEM_DETAIL_TRACE_A("Copying headers.");

    std::vector<std::uint8_t> raw_new;
    std::copy(std::begin(raw),
              std::begin(raw) + nt_headers.GetSizeOfHeaders(),
              std::back_inserter(raw_new));

    HADESMEM_DETAIL_TRACE_A("Copying section data.");

    hadesmem::SectionList const sections(local_process, pe_file);
    std::vector<std::pair<DWORD, DWORD>> raw_datas;
    for (auto const& section : sections)
    {
      auto const section_size =
        (std::max)(section.GetVirtualSize(), section.GetSizeOfRawData());
      auto const ptr_raw_data_new =
        section.GetPointerToRawData() < raw_new.size()
          ? static_cast<DWORD>(
              RoundUp(raw_new.size(), nt_headers.GetFileAlignment()))
          : section.GetPointerToRawData();
      raw_datas.emplace_back(ptr_raw_data_new, section_size);

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
    nt_headers_new.SetImageBase(
      reinterpret_cast<ULONG_PTR>(module.GetHandle()));
    nt_headers_new.UpdateWrite();

    HADESMEM_DETAIL_TRACE_A("Fixing section headers.");

    hadesmem::SectionList sections_new(local_process, pe_file_new);
    std::size_t n = 0;
    for (auto& section : sections_new)
    {
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
  void* base = nullptr,
  std::size_t size = 0)
{
  if (!base && !size)
  {
    DumpAllModules(process);
  }
  else
  {
    DumpMemoryRegionRaw(process, base, size);
  }
}
}
}
