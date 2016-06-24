// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/detail/smart_handle.hpp>

#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/privilege.hpp>
#include <hadesmem/detail/scope_warden.hpp>
#include <hadesmem/detail/trace.hpp>

namespace hadesmem
{
inline void GetSeLoadDriverPrivilege()
{
  return detail::GetPrivilege(SE_LOAD_DRIVER_NAME);
}

inline void LoadDriver(std::wstring const& name, std::wstring const& path)
{
  if (detail::IsPathRelative(path))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"Path must be absolute."});
  }

  auto const ntdll = ::GetModuleHandleW(L"ntdll.dll");
  if (!ntdll)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"GetModuleHandleW failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  typedef NTSTATUS(NTAPI * NtLoadDriverFn)(PUNICODE_STRING DriverServiceName);

  auto const nt_load_driver =
    reinterpret_cast<NtLoadDriverFn>(::GetProcAddress(ntdll, "NtLoadDriver"));
  if (!nt_load_driver)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"GetProcAddress failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  HKEY services_key = nullptr;
  auto const open_status = ::RegOpenKeyW(
    HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services", &services_key);
  if (open_status != ERROR_SUCCESS)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"RegOpenKeyW failed."}
                                    << ErrorCodeWinLast{last_error}
                                    << ErrorCodeWinHr{open_status});
  }
  hadesmem::detail::SmartRegKeyHandle const smart_services_key{services_key};

  HKEY service_key = nullptr;
  DWORD disposition = 0;
  auto const create_status = ::RegCreateKeyExW(services_key,
                                               name.c_str(),
                                               0,
                                               nullptr,
                                               0,
                                               KEY_ALL_ACCESS,
                                               nullptr,
                                               &service_key,
                                               &disposition);
  if (create_status != ERROR_SUCCESS)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"RegCreateKeyW failed."}
                                    << ErrorCodeWinLast{last_error}
                                    << ErrorCodeWinHr{create_status});
  }
  hadesmem::detail::SmartRegKeyHandle const smart_service_key{service_key};

  if (disposition == REG_OPENED_EXISTING_KEY)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"Target registry key already exists."});
  }

  auto const delete_tree = [&]() {
    ::RegDeleteTreeW(HKEY_LOCAL_MACHINE,
                     (L"SYSTEM\\CurrentControlSet\\Services\\" + name).c_str());
  };

  auto ensure_delete_tree = hadesmem::detail::MakeScopeWarden(delete_tree);

  auto const image_path = L"\\??\\" + path;
  auto const value_path_status = ::RegSetValueExW(
    service_key,
    L"ImagePath",
    0,
    REG_SZ,
    reinterpret_cast<BYTE const*>(image_path.c_str()),
    static_cast<DWORD>((image_path.size() + 1) * sizeof(wchar_t)));
  if (value_path_status != ERROR_SUCCESS)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"RegSetValueExW failed."}
                                    << ErrorCodeWinLast{last_error}
                                    << ErrorCodeWinHr{value_path_status});
  }

  DWORD const type = 1;
  auto const value_type_status =
    RegSetValueExW(service_key,
                   L"Type",
                   0,
                   REG_DWORD,
                   reinterpret_cast<const BYTE*>(&type),
                   sizeof(type));
  if (value_type_status != ERROR_SUCCESS)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"RegSetValueExW failed."}
                                    << ErrorCodeWinLast{last_error}
                                    << ErrorCodeWinHr{value_type_status});
  }

  UNICODE_STRING reg_path;
  std::wstring const reg_path_buf =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + name;
  ::RtlInitUnicodeString(&reg_path, reg_path_buf.c_str());
  auto const load_status = nt_load_driver(&reg_path);
  if (!NT_SUCCESS(load_status))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"NtLoadDriver failed."}
                                    << ErrorCodeWinStatus{load_status});
  }

  ensure_delete_tree.Dismiss();
}

inline void UnloadDriver(std::wstring const& name)
{
  auto const ntdll = ::GetModuleHandleW(L"ntdll.dll");
  if (!ntdll)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"GetModuleHandleW failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  typedef NTSTATUS(NTAPI * NtUnloadDriverFn)(PUNICODE_STRING DriverServiceName);

  auto const nt_unload_driver = reinterpret_cast<NtUnloadDriverFn>(
    ::GetProcAddress(ntdll, "NtUnloadDriver"));
  if (!nt_unload_driver)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"GetProcAddress failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  UNICODE_STRING reg_path;
  std::wstring const reg_path_buf =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + name;
  ::RtlInitUnicodeString(&reg_path, reg_path_buf.c_str());
  auto const unload_status = nt_unload_driver(&reg_path);
  if (!NT_SUCCESS(unload_status))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"NtUnloadDriver failed."}
                                    << ErrorCodeWinStatus{unload_status});
  }

  auto const reg_status =
    ::RegDeleteTreeW(HKEY_LOCAL_MACHINE,
                     (L"SYSTEM\\CurrentControlSet\\Services\\" + name).c_str());
  if (reg_status != ERROR_SUCCESS)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"RegDeleteTreeW failed."}
                                    << ErrorCodeWinLast{last_error}
                                    << ErrorCodeWinHr{reg_status});
  }
}
}
