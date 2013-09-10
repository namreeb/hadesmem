// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <ostream>
#include <utility>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/winapi.hpp>
#include <hadesmem/detail/smart_handle.hpp>

// TODO: Support cross architecture process manipulation (opening an x86 
// WoW64 process as native x64). Includes removing dependency of non-ntdll 
// in injected code and DLL loader, and adding cross architecture support 
// for pelib.

namespace hadesmem
{

class Process
{
public:
  explicit Process(DWORD id)
    : handle_(detail::OpenProcessAllAccess(id)), 
    id_(id)
  {
    CheckWoW64();
  }
  
  Process(Process const& other)
    : handle_(detail::DuplicateHandle(other.handle_.GetHandle())), 
    id_(other.id_)
  { }
  
  Process& operator=(Process const& other)
  {
    handle_ = detail::DuplicateHandle(other.handle_.GetHandle());
    id_ = other.id_;
  
    return *this;
  }
  
  Process(Process&& other) HADESMEM_NOEXCEPT
    : handle_(std::move(other.handle_)), 
    id_(other.id_)
  {
    other.id_ = 0;
  }
  
  Process& operator=(Process&& other) HADESMEM_NOEXCEPT
  {
    handle_ = std::move(other.handle_);
    id_ = other.id_;

    other.id_ = 0;
  
    return *this;
  }
  
  ~Process() HADESMEM_NOEXCEPT
  {
    CleanupUnchecked();
  }
  
  DWORD GetId() const HADESMEM_NOEXCEPT
  {
    return id_;
  }
  
  HANDLE GetHandle() const HADESMEM_NOEXCEPT
  {
    return handle_.GetHandle();
  }
  
  void Cleanup()
{
  handle_.Cleanup();
  id_ = 0;
}
  
private:
  void CheckWoW64() const
  {
    if (detail::IsWoW64Process(::GetCurrentProcess()) != 
      detail::IsWoW64Process(handle_.GetHandle()))
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Cross-architecture process manipulation is currently "
        "unsupported."));
    }
  }

  void CleanupUnchecked() HADESMEM_NOEXCEPT
  {
    try
    {
      Cleanup();
    }
    catch (std::exception const& e)
    {
      (void)e;

      // WARNING: Handle is leaked if 'Cleanup' fails.
      HADESMEM_DETAIL_ASSERT(false);

      id_ = 0;
      handle_ = nullptr;
    }
  }

  detail::SmartHandle handle_;
  DWORD id_;
};

inline bool operator==(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetId() == rhs.GetId();
}

inline bool operator!=(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetId() < rhs.GetId();
}

inline bool operator<=(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetId() <= rhs.GetId();
}

inline bool operator>(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetId() > rhs.GetId();
}

inline bool operator>=(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetId() >= rhs.GetId();
}

inline std::ostream& operator<<(std::ostream& lhs, Process const& rhs)
{
  std::locale old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetId();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Process const& rhs)
{
  std::locale old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetId();
  lhs.imbue(old);
  return lhs;
}

// TODO: Move this to a more appropriate header (process_helpers.hpp?).
inline std::wstring GetPath(Process const& process)
{
  std::vector<wchar_t> path(HADESMEM_MAX_PATH_UNICODE);
  DWORD path_len = static_cast<DWORD>(path.size());
  if (!::QueryFullProcessImageNameW(process.GetHandle(), 0, path.data(), 
    &path_len))
  {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("QueryFullProcessImageName failed.") << 
        ErrorCodeWinLast(last_error));
  }
  
  return path.data();
}

// TODO: Move this to a more appropriate header (process_helpers.hpp?).
inline bool IsWoW64(Process const& process)
{
  return detail::IsWoW64Process(process.GetHandle());
}

// TODO: Move this to a more appropriate header (get_debug_privilege.hpp?).
inline void GetSeDebugPrivilege()
{
  HANDLE process_token_temp = 0;
  if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | 
    TOKEN_QUERY, &process_token_temp)) 
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("OpenProcessToken failed.") << 
      ErrorCodeWinLast(last_error));
  }
  detail::SmartHandle const process_token(process_token_temp);

  LUID luid = { 0, 0 };
  if (!::LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("LookupPrivilegeValue failed.") << 
      ErrorCodeWinLast(last_error));
  }

  TOKEN_PRIVILEGES privileges;
  ::ZeroMemory(&privileges, sizeof(privileges));
  privileges.PrivilegeCount = 1;
  privileges.Privileges[0].Luid = luid;
  privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if (!::AdjustTokenPrivileges(process_token.GetHandle(), FALSE, &privileges, 
    static_cast<DWORD>(sizeof(privileges)), nullptr, nullptr) || 
    ::GetLastError() == ERROR_NOT_ALL_ASSIGNED) 
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("AdjustTokenPrivileges failed.") << 
      ErrorCodeWinLast(last_error));
  }
}

}
