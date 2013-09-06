// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <vector>
#include <ostream>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/winapi.hpp>
#include <hadesmem/detail/smart_handle.hpp>

// TODO: Add tests for Thread.

// TODO: Support interacting with WoW64 threads from x64 procs (via 
// Wow64SuspendThread, Wow64GetThreadContext, etc.).

namespace hadesmem
{

class Thread
{
public:
  explicit Thread(DWORD id)
    : handle_(detail::OpenThreadAllAccess(id)), 
    id_(id)
  { }
  
  Thread(Thread const& other)
    : handle_(detail::DuplicateHandle(other.handle_.GetHandle())), 
    id_(other.id_)
  { }
  
  Thread& operator=(Thread const& other)
  {
    handle_ = detail::DuplicateHandle(other.handle_.GetHandle());
    id_ = other.id_;
  
    return *this;
  }
  
  Thread(Thread&& other) HADESMEM_NOEXCEPT
    : handle_(std::move(other.handle_)), 
    id_(other.id_)
  {
    other.id_ = 0;
  }
  
  Thread& operator=(Thread&& other) HADESMEM_NOEXCEPT
  {
    handle_ = std::move(other.handle_);
    id_ = other.id_;

    other.id_ = 0;
  
    return *this;
  }
  
  ~Thread() HADESMEM_NOEXCEPT
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
      HADESMEM_ASSERT(boost::diagnostic_information(e).c_str() && false);

      id_ = 0;
      handle_ = nullptr;
    }
  }

  detail::SmartHandle handle_;
  DWORD id_;
};

inline bool operator==(Thread const& lhs, Thread const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetId() == rhs.GetId();
}

inline bool operator!=(Thread const& lhs, Thread const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Thread const& lhs, Thread const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetId() < rhs.GetId();
}

inline bool operator<=(Thread const& lhs, Thread const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetId() <= rhs.GetId();
}

inline bool operator>(Thread const& lhs, Thread const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetId() > rhs.GetId();
}

inline bool operator>=(Thread const& lhs, Process const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetId() >= rhs.GetId();
}

inline std::ostream& operator<<(std::ostream& lhs, Thread const& rhs)
{
  std::locale old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetId();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Thread const& rhs)
{
  std::locale old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetId();
  lhs.imbue(old);
  return lhs;
}

}
