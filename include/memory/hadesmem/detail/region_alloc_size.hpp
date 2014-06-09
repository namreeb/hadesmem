// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <iterator>
#include <limits>

#include <windows.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/region_list.hpp>

namespace hadesmem
{

namespace detail
{

inline SIZE_T GetRegionAllocSize(hadesmem::Process const& process,
                                 void const* base)
{
  HMODULE const ntdll = ::GetModuleHandleW(L"ntdll.dll");
  if (!ntdll)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"GetModuleHandleW failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  using FnNtQueryInformationProcess =
    NTSTATUS(NTAPI*)(HANDLE process,
                     PROCESSINFOCLASS info_class,
                     PVOID info,
                     ULONG info_length,
                     PULONG return_length);
  auto const nt_query_information_process =
    reinterpret_cast<FnNtQueryInformationProcess>(
      GetProcAddress(ntdll, "NtQueryInformationProcess"));
  if (!nt_query_information_process)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"NtQueryInformationProcess failed."}
              << ErrorCodeWinLast{last_error});
  }

  PROCESS_BASIC_INFORMATION pbi{};
  NTSTATUS const query_peb_result =
    nt_query_information_process(process.GetHandle(),
                                 ProcessBasicInformation,
                                 &pbi,
                                 static_cast<ULONG>(sizeof(pbi)),
                                 nullptr);
  if (!NT_SUCCESS(query_peb_result))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"NtQueryInformationProcess failed."}
              << ErrorCodeWinStatus{query_peb_result});
  }

  // The technique we're using will not work to get the size of images mapped
  // with large pages (the start address of the mapping is randomized).
  auto const peb = Read<winternl::PEB>(process, pbi.PebBaseAddress);
  if (!!(peb.BitField & 1))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{
        "GetRegionAllocSize does not currently support large pages."});
  }

  hadesmem::RegionList regions{process};
  auto iter = std::find_if(std::begin(regions),
                           std::end(regions),
                           [&](Region const& region)
                           {
    return region.GetAllocBase() == base;
  });
  SIZE_T size{};
  while (iter != std::end(regions) && iter->GetAllocBase() == base)
  {
    SIZE_T const region_size = iter->GetSize();
    HADESMEM_DETAIL_ASSERT(region_size < (std::numeric_limits<DWORD>::max)());
    size += static_cast<DWORD>(region_size);
    HADESMEM_DETAIL_ASSERT(size >= region_size);
    ++iter;
  }
  if (!size)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"Invalid region allocation size."});
  }
  return size;
}
}
}
