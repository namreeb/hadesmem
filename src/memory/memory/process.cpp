// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/process.hpp"

#include <array>
#include <utility>
#include <iostream>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include <boost/scope_exit.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"

namespace hadesmem
{

Process::Process(DWORD id)
  : id_(id), 
  handle_(Open(id))
{
  CheckWoW64();
}

Process::Process(Process const& other)
  : id_(0), 
  handle_(nullptr)
{
  HANDLE const new_handle = Duplicate(other.handle_);
  
  handle_ = new_handle;
  id_ = other.id_;
}

Process& Process::operator=(Process const& other)
{
  HANDLE const new_handle = Duplicate(other.handle_);
  
  Cleanup();
  
  handle_ = new_handle;
  id_ = other.id_;
  
  return *this;
}

Process::Process(Process&& other) BOOST_NOEXCEPT
  : id_(other.id_), 
  handle_(other.handle_)
{
  other.id_ = 0;
  other.handle_ = nullptr;
}

Process& Process::operator=(Process&& other) BOOST_NOEXCEPT
{
  Cleanup();
  
  id_ = other.id_;
  handle_ = other.handle_;
  
  other.id_ = 0;
  other.handle_ = nullptr;
  
  return *this;
}

Process::~Process()
{
  CleanupUnchecked();
}

DWORD Process::GetId() const BOOST_NOEXCEPT
{
  return id_;
}

HANDLE Process::GetHandle() const BOOST_NOEXCEPT
{
  return handle_;
}

void Process::Cleanup()
{
  if (!handle_)
  {
    return;
  }
  
  if (!::CloseHandle(handle_))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("CloseHandle failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  id_ = 0;
  handle_ = nullptr;
}

void Process::CheckWoW64() const
{
  BOOL is_wow64_me = FALSE;
  if (!::IsWow64Process(GetCurrentProcess(), &is_wow64_me))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not detect WoW64 status of current process.") << 
      ErrorCodeWinLast(last_error));
  }
  
  BOOL is_wow64 = FALSE;
  if (!::IsWow64Process(handle_, &is_wow64))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not detect WoW64 status of target process.") << 
      ErrorCodeWinLast(last_error));
  }
  
  if (is_wow64_me != is_wow64)
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Cross-architecture process manipulation is currently "
        "unsupported."));
  }
}

HANDLE Process::Open(DWORD id)
{
  HANDLE const handle = ::OpenProcess(PROCESS_ALL_ACCESS, TRUE, id);
  if (!handle)
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("OpenProcess failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return handle;
}

void Process::CleanupUnchecked() BOOST_NOEXCEPT
{
  try
  {
    Cleanup();
  }
  catch (std::exception const& e)
  {
    (void)e;
    
    // WARNING: Handle is leaked if 'Cleanup' fails.
    BOOST_ASSERT_MSG(false, boost::diagnostic_information(e).c_str());
    
    id_ = 0;
    handle_ = nullptr;
  }
}

HANDLE Process::Duplicate(HANDLE handle)
{
  BOOST_ASSERT(handle != nullptr);
  
  HANDLE new_handle = nullptr;
  if (!::DuplicateHandle(::GetCurrentProcess(), handle, 
    ::GetCurrentProcess(), &new_handle, 0, TRUE, DUPLICATE_SAME_ACCESS))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("DuplicateHandle failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return new_handle;
}

bool operator==(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetId() == rhs.GetId();
}

bool operator!=(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetId() < rhs.GetId();
}

bool operator<=(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetId() <= rhs.GetId();
}

bool operator>(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetId() > rhs.GetId();
}

bool operator>=(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetId() >= rhs.GetId();
}

std::ostream& operator<<(std::ostream& lhs, Process const& rhs)
{
  return (lhs << rhs.GetId());
}

std::wostream& operator<<(std::wostream& lhs, Process const& rhs)
{
  return (lhs << rhs.GetId());
}

std::wstring GetPath(Process const& process)
{
  std::array<wchar_t, MAX_PATH> path = { { 0 } };
  DWORD path_len = static_cast<DWORD>(path.size());
  if (!::QueryFullProcessImageName(process.GetHandle(), 0, path.data(), 
    &path_len))
  {
      DWORD const last_error = ::GetLastError();
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("QueryFullProcessImageName failed.") << 
        ErrorCodeWinLast(last_error));
  }
  
  return path.data();
}

bool IsWoW64(Process const& process)
{
  BOOL is_wow64 = FALSE;
  if (!::IsWow64Process(process.GetHandle(), &is_wow64))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("IsWow64Process failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return is_wow64 != FALSE;
}

void GetSeDebugPrivilege()
{
  HANDLE process_token = 0;
  if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | 
    TOKEN_QUERY, &process_token)) 
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("OpenProcessToken failed.") << 
      ErrorCodeWinLast(last_error));
  }

  BOOST_SCOPE_EXIT_ALL(&)
  {
    // WARNING: Handle is leaked if CloseHandle fails.
    BOOST_VERIFY(::CloseHandle(process_token));
  };

  LUID luid = { 0, 0 };
  if (!::LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("LookupPrivilegeValue failed.") << 
      ErrorCodeWinLast(last_error));
  }

  TOKEN_PRIVILEGES privileges;
  ::ZeroMemory(&privileges, sizeof(privileges));
  privileges.PrivilegeCount = 1;
  privileges.Privileges[0].Luid = luid;
  privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if (!::AdjustTokenPrivileges(process_token, FALSE, &privileges, 
    sizeof(privileges), nullptr, nullptr) || 
    ::GetLastError() == ERROR_NOT_ALL_ASSIGNED) 
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("AdjustTokenPrivileges failed.") << 
      ErrorCodeWinLast(last_error));
  }
}

}
