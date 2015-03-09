// Copyright (C) 2010-2014 Joshua Boyce.
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

  Process(Process&& other) HADESMEM_DETAIL_NOEXCEPT
    : handle_{std::move(other.handle_)},
      id_{other.id_}
  {
    other.id_ = 0;
  }

  Process& operator=(Process&& other) HADESMEM_DETAIL_NOEXCEPT
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

  DWORD GetId() const HADESMEM_DETAIL_NOEXCEPT
  {
    return id_;
  }

  HANDLE GetHandle() const HADESMEM_DETAIL_NOEXCEPT
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
    if (detail::IsWoW64Process(::GetCurrentProcess()) !=
        detail::IsWoW64Process(handle_.GetHandle()))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Cross-architecture process manipulation is "
                               "currently unsupported."});
    }
  }

  void CleanupUnchecked() HADESMEM_DETAIL_NOEXCEPT
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

inline bool operator==(Process const& lhs,
                       Process const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetId() == rhs.GetId();
}

inline bool operator!=(Process const& lhs,
                       Process const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Process const& lhs,
                      Process const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetId() < rhs.GetId();
}

inline bool operator<=(Process const& lhs,
                       Process const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetId() <= rhs.GetId();
}

inline bool operator>(Process const& lhs,
                      Process const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetId() > rhs.GetId();
}

inline bool operator>=(Process const& lhs,
                       Process const& rhs) HADESMEM_DETAIL_NOEXCEPT
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
