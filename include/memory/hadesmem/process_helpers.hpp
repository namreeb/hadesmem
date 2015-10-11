// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/detail/winapi.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/process_list.hpp>

namespace hadesmem
{
inline ProcessEntry GetProcessEntryByName(std::wstring const& proc_name,
                                          bool name_forced = false)
{
  std::wstring const proc_name_upper =
    hadesmem::detail::ToUpperOrdinal(proc_name);
  auto const compare_proc_name = [&](hadesmem::ProcessEntry const& proc_entry)
  {
    return hadesmem::detail::ToUpperOrdinal(proc_entry.GetName()) ==
           proc_name_upper;
  };
  hadesmem::ProcessList proc_list;
  if (name_forced)
  {
    auto const proc_iter = std::find_if(
      std::begin(proc_list), std::end(proc_list), compare_proc_name);
    if (proc_iter == std::end(proc_list))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"Failed to find process."});
    }

    return *proc_iter;
  }
  else
  {
    std::vector<hadesmem::ProcessEntry> found_procs;
    std::copy_if(std::begin(proc_list),
                 std::end(proc_list),
                 std::back_inserter(found_procs),
                 compare_proc_name);

    if (found_procs.empty())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"Failed to find process."});
    }

    if (found_procs.size() > 1)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
          "Process name search found multiple matches."});
    }

    return found_procs.front();
  }
}

inline Process GetProcessByName(std::wstring const& proc_name,
                                bool name_forced = false)
{
  // Guard against potential PID reuse race condition. Unlikely
  // to ever happen in practice, but better safe than sorry.
  DWORD retries = 3;
  do
  {
    hadesmem::Process process_1{
      GetProcessEntryByName(proc_name, name_forced).GetId()};
    hadesmem::Process process_2{
      GetProcessEntryByName(proc_name, name_forced).GetId()};
    if (process_1 == process_2)
    {
      return process_1;
    }
  } while (retries--);

  HADESMEM_DETAIL_THROW_EXCEPTION(
    hadesmem::Error{} << hadesmem::ErrorString{
      "Could not get handle to target process (PID reuse race)."});
}

inline std::wstring GetPathNative(Process const& process)
{
  HMODULE const ntdll = GetModuleHandleW(L"ntdll");
  if (!ntdll)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"GetModuleHandleW failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  FARPROC const nt_query_system_information_proc =
    GetProcAddress(ntdll, "NtQuerySystemInformation");
  if (!nt_query_system_information_proc)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"GetProcAddress failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  using NtQuerySystemInformationPtr = NTSTATUS(
    NTAPI*)(detail::winternl::SYSTEM_INFORMATION_CLASS system_information_class,
            PVOID system_information,
            ULONG system_information_length,
            PULONG return_length);

  auto const nt_query_system_information =
    reinterpret_cast<NtQuerySystemInformationPtr>(
      nt_query_system_information_proc);

  std::vector<wchar_t> buffer(HADESMEM_DETAIL_MAX_PATH_UNICODE);
  detail::winternl::SYSTEM_PROCESS_ID_INFORMATION process_id_info;
  process_id_info.ProcessId =
    reinterpret_cast<void*>(static_cast<DWORD_PTR>(process.GetId()));
  process_id_info.ImageName.Buffer = buffer.data();
  process_id_info.ImageName.Length = 0;
  process_id_info.ImageName.MaximumLength = static_cast<USHORT>(buffer.size());

  NTSTATUS const status =
    nt_query_system_information(detail::winternl::SystemProcessIdInformation,
                                &process_id_info,
                                sizeof(process_id_info),
                                nullptr);
  if (!NT_SUCCESS(status))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"NtQuerySystemInformation failed."}
              << ErrorCodeWinStatus{status});
  }

  return {buffer.data(), buffer.data() + process_id_info.ImageName.Length / 2};
}

// TODO: Use GetPathNative and convert the result to a Win32 path, because it
// bypasses access restrictions and also works for 'zombie' processes, where
// QueryFullProcessImageNameW fails with ERROR_GEN_FAILURE.
inline std::wstring GetPath(Process const& process)
{
  return detail::QueryFullProcessImageNameW(process.GetHandle());
}

inline bool IsWoW64(Process const& process)
{
  return detail::IsWoW64Process(process.GetHandle());
}
}
