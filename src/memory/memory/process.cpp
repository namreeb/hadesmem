// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/process.hpp"

#include <array>
#include <utility>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
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
  
  std::swap(this->id_, other.id_);
  std::swap(this->handle_, other.handle_);
  
  return *this;
}

Process::Process(Process const& other)
  : id_(0), 
  handle_(nullptr)
{
  HANDLE new_handle = nullptr;
  if (!DuplicateHandle(GetCurrentProcess(), other.handle_, 
    GetCurrentProcess(), &new_handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("DuplicateHandle failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  handle_ = new_handle;
  id_ = other.id_;
}

Process& Process::operator=(Process const& other)
{
  HANDLE new_handle = nullptr;
  if (!DuplicateHandle(GetCurrentProcess(), other.handle_, 
    GetCurrentProcess(), &new_handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("DuplicateHandle failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  Cleanup();
  
  handle_ = new_handle;
  
  id_ = other.id_;
  
  return *this;
}

Process::~Process() BOOST_NOEXCEPT
{
  if (handle_ && handle_ != GetCurrentProcess())
  {
    BOOST_VERIFY(CloseHandle(handle_));
  }
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
  
  BOOST_ASSERT(id_);
  
  if (!CloseHandle(handle_))
  {
    DWORD const last_error = GetLastError();
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
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not detect WoW64 status of current process.") << 
      ErrorCodeWinLast(last_error));
  }
  
  BOOL is_wow64 = FALSE;
  if (!::IsWow64Process(handle_, &is_wow64))
  {
    DWORD const last_error = GetLastError();
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
  HANDLE handle = ::OpenProcess(PROCESS_CREATE_THREAD | 
    PROCESS_QUERY_INFORMATION | 
    PROCESS_VM_OPERATION | 
    PROCESS_VM_READ | 
    PROCESS_VM_WRITE, 
    FALSE, 
    id);
  if (!handle)
  {
    DWORD const last_error = GetLastError();
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
    // WARNING: Handle is leaked if 'Cleanup' fails.
    BOOST_ASSERT_MSG(false, boost::diagnostic_information(e).c_str());
    
    id_ = 0;
    handle_ = nullptr;
  }
}

bool Process::operator==(Process const& other) const BOOST_NOEXCEPT
{
  return this->id_ == other.id_;
}

bool Process::operator!=(Process const& other) const BOOST_NOEXCEPT
{
  return !(*this == other);
}

std::wstring GetPath(Process const& process)
{
  std::array<wchar_t, MAX_PATH> path = { { } };
  DWORD path_len = static_cast<DWORD>(path.size());
  if (!::QueryFullProcessImageName(process.GetHandle(), 0, path.data(), 
    &path_len))
  {
      DWORD const last_error = GetLastError();
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
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("IsWow64Process failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return is_wow64 != FALSE;
}

}
