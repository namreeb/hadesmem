// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/detail/smart_handle.hpp>

namespace hadesmem
{
namespace detail
{
inline void GetPrivilege(std::wstring const& name)
{
  HANDLE process_token_temp = nullptr;
  if (!::OpenProcessToken(::GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &process_token_temp))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"OpenProcessToken failed."}
                                    << ErrorCodeWinLast{last_error});
  }
  detail::SmartHandle const process_token{process_token_temp};

  LUID luid = {0, 0};
  if (!::LookupPrivilegeValueW(nullptr, name.c_str(), &luid))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"LookupPrivilegeValue failed."}
              << ErrorCodeWinLast{last_error});
  }

  TOKEN_PRIVILEGES privileges{};
  privileges.PrivilegeCount = 1;
  privileges.Privileges[0].Luid = luid;
  privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if (!::AdjustTokenPrivileges(process_token.GetHandle(),
                               FALSE,
                               &privileges,
                               static_cast<DWORD>(sizeof(privileges)),
                               nullptr,
                               nullptr) ||
      ::GetLastError() == ERROR_NOT_ALL_ASSIGNED)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"AdjustTokenPrivileges failed."}
              << ErrorCodeWinLast{last_error});
  }
}
}
}
