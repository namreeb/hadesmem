// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/process.hpp"

#include <array>
#include <utility>

#if defined(HADESMEM_MSVC)
#pragma warning(push, 1)
#pragma warning(disable: 4996)
#endif // #if defined(HADESMEM_MSVC)
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#endif // #if defined(HADESMEM_GCC)
#include <boost/locale.hpp>
#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

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

Process::~Process() BOOST_NOEXCEPT
{
  Cleanup();
}

DWORD Process::GetId() const BOOST_NOEXCEPT
{
  return id_;
}

HANDLE Process::GetHandle() const BOOST_NOEXCEPT
{
  return handle_;
}

void Process::CheckWoW64() const
{
  BOOL is_wow64_me = FALSE;
  if (!IsWow64Process(GetCurrentProcess(), &is_wow64_me))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorFunction("Process::CheckWoW64") << 
      ErrorString("Could not detect WoW64 status of current process.") << 
      ErrorCodeWinLast(last_error));
  }
  
  BOOL is_wow64 = FALSE;
  if (!IsWow64Process(handle_, &is_wow64))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorFunction("Process::CheckWoW64") << 
      ErrorString("Could not detect WoW64 status of target process.") << 
      ErrorCodeWinLast(last_error));
  }
  
  if (is_wow64_me != is_wow64)
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorFunction("Process::CheckWoW64") << 
      ErrorString("Cross-architecture process manipulation is currently "
        "unsupported."));
  }
}

HANDLE Process::Open(DWORD id)
{
  HANDLE handle = OpenProcess(PROCESS_CREATE_THREAD | 
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
      ErrorFunction("Process::Open") << 
      ErrorString("OpenProcess failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return handle;
}

void Process::Cleanup()
{
  if (handle_)
  {
    BOOST_ASSERT(CloseHandle(handle_));
  }
  
  id_ = 0;
  handle_ = nullptr;
}

bool operator==(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetId() == rhs.GetId();
}

bool operator!=(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT
{
  return !(lhs == rhs);
}

std::string GetPath(Process const& process)
{
  std::array<wchar_t, MAX_PATH> path = { { } };
  DWORD path_len = static_cast<DWORD>(path.size());
  if (!QueryFullProcessImageName(process.GetHandle(), 0, path.data(), 
    &path_len))
  {
      DWORD const last_error = GetLastError();
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorFunction("GetPath") << 
        ErrorString("QueryFullProcessImageName failed.") << 
        ErrorCodeWinLast(last_error));
  }
  
  return boost::locale::conv::utf_to_utf<char>(path.data());
}

bool IsWoW64(Process const& process)
{
  BOOL is_wow64 = FALSE;
  if (!IsWow64Process(process.GetHandle(), &is_wow64))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorFunction("IsWoW64") << 
      ErrorString("IsWow64Process failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return is_wow64 != FALSE;
}

}
