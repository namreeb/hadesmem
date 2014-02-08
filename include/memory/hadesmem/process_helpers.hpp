// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/winapi.hpp>
#include <hadesmem/detail/winternl.hpp>

namespace hadesmem
{

inline std::wstring GetPathNative(Process const& process)
{
  HMODULE const ntdll = GetModuleHandleW(L"ntdll");
  if (!ntdll)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("GetModuleHandleW failed.")
                                    << ErrorCodeWinLast(last_error));
  }

  FARPROC const nt_query_system_information_proc =
    GetProcAddress(ntdll, "NtQuerySystemInformation");
  if (!nt_query_system_information_proc)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("GetProcAddress failed.")
                                    << ErrorCodeWinLast(last_error));
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
  process_id_info.ProcessId = reinterpret_cast<void*>(process.GetId());
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
      Error() << ErrorString("NtQuerySystemInformation failed.")
              << ErrorCodeWinStatus(status));
  }

  return {buffer.data(), buffer.data() + process_id_info.ImageName.Length / 2};
}

// TODO: Use GetPathNative and then convert the path to a Win32 path instead,
// because it bypasses access restrictions and also works for 'zombie'
// processes, where QueryFullProcessImageName fails with ERROR_GEN_FAILURE.
inline std::wstring GetPath(Process const& process)
{
  return detail::QueryFullProcessImageNameW(process.GetHandle());
}

inline bool IsWoW64(Process const& process)
{
  return detail::IsWoW64Process(process.GetHandle());
}
}
