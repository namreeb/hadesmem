// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/winapi.hpp>
#include <hadesmem/error.hpp>

// TODO: Process should keep track of its access mask (beginning with the least
// amount of access possible - i.e. PROCESS_QUERY_LIMITED_INFORMATION). When a
// components wants to perform an operation on a process (e.g. read memory) it
// should first call a function with the required access mask to ensure that it
// is available, and reopen the handle if not. This way we can keep privs as
// limited as possible, which will be useful when we can only get limited access
// to a process.

// TODO: Support cross architecture process manipulation (opening an x86 WoW64
// process as native x64). Includes removing dependency of non-ntdll in injected
// code and DLL loader, and adding cross architecture support for pelib, thread,
// etc. We have removed the x64 -> x86 hard-restriction, but there are still
// many components that need updating. Many components need special thought such
// as dump, call, inject, patch, findpattern, etc. We could probably re-use the
// cross-arch inject workaround from Cerberus until we finish adding full native
// support.

namespace hadesmem
{
class Process
{
public:
  explicit Process(DWORD id) : handle_{OpenProcess(id)}, id_{id}
  {
    CheckWoW64();
  }

  Process(Process const& other)
    : handle_{DuplicateHandle(other.id_, other.handle_.GetHandle())},
      id_{other.id_}
  {
  }

  Process& operator=(Process const& other)
  {
    Process tmp{other};
    *this = std::move(tmp);

    return *this;
  }

  Process(Process&& other) noexcept : handle_{std::move(other.handle_)},
                                      id_{other.id_}
  {
    other.id_ = 0;
  }

  Process& operator=(Process&& other) noexcept
  {
    CleanupUnchecked();

    handle_ = std::move(other.handle_);
    id_ = other.id_;

    other.id_ = 0;

    return *this;
  }

  ~Process()
  {
    CleanupUnchecked();
  }

  DWORD GetId() const noexcept
  {
    return id_;
  }

  HANDLE GetHandle() const noexcept
  {
    return handle_.GetHandle();
  }

  void Cleanup()
  {
    if (id_ != ::GetCurrentProcessId())
    {
      handle_.Cleanup();
    }
    else
    {
      handle_.Detach();
    }

    id_ = 0;
  }

private:
  void CheckWoW64() const
  {
    if (detail::IsWoW64Process(::GetCurrentProcess()) &&
        !detail::IsWoW64Process(handle_.GetHandle()))
    {
      // TODO: Lift this restriction. We can use NtWow64ReadVirtualMemory64 (and
      // similar APIs for thread context, process information query, etc.) to
      // make it work, but we will need to implement more APIs manually (e.g.
      // module enumeration?).
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"x86 -> x64 process modification unsupported."});
    }
  }

  void CleanupUnchecked() noexcept
  {
    try
    {
      Cleanup();
    }
    catch (...)
    {
      // WARNING: Handle is leaked if 'Cleanup' fails.
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);

      id_ = 0;
      handle_ = nullptr;
    }
  }

  HANDLE OpenProcess(DWORD id) const
  {
    return id == ::GetCurrentProcessId()
             ? ::GetCurrentProcess()
             : detail::OpenProcessAllAccess(id).Detach();
  }

  HANDLE DuplicateHandle(DWORD id, HANDLE handle) const
  {
    return id == ::GetCurrentProcessId()
             ? ::GetCurrentProcess()
             : detail::DuplicateHandle(handle).Detach();
  }

  detail::SmartHandle handle_;
  DWORD id_;
};

inline bool operator==(Process const& lhs, Process const& rhs) noexcept
{
  return lhs.GetId() == rhs.GetId();
}

inline bool operator!=(Process const& lhs, Process const& rhs) noexcept
{
  return !(lhs == rhs);
}

inline bool operator<(Process const& lhs, Process const& rhs) noexcept
{
  return lhs.GetId() < rhs.GetId();
}

inline bool operator<=(Process const& lhs, Process const& rhs) noexcept
{
  return lhs.GetId() <= rhs.GetId();
}

inline bool operator>(Process const& lhs, Process const& rhs) noexcept
{
  return lhs.GetId() > rhs.GetId();
}

inline bool operator>=(Process const& lhs, Process const& rhs) noexcept
{
  return lhs.GetId() >= rhs.GetId();
}

inline std::ostream& operator<<(std::ostream& lhs, Process const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetId();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Process const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetId();
  lhs.imbue(old);
  return lhs;
}
}
