// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

namespace hadesmem
{
namespace detail
{
inline winternl::PEB GetPeb(Process const& process)
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

  return Read<winternl::PEB>(process, pbi.PebBaseAddress);
}
}
}
