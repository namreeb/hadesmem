// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
class ThreadEntry
{
public:
  constexpr explicit ThreadEntry(THREADENTRY32 const& entry)
    noexcept : usage_{entry.cntUsage},
                               thread_id_{entry.th32ThreadID},
                               owner_process_id_{entry.th32OwnerProcessID},
                               base_priority_{entry.tpBasePri},
                               delta_priority_{entry.tpDeltaPri},
                               flags_{entry.dwFlags}
  {
  }

  constexpr DWORD GetUsage() const noexcept
  {
    return usage_;
  }

  constexpr DWORD GetId() const noexcept
  {
    return thread_id_;
  }

  constexpr DWORD GetOwnerId() const noexcept
  {
    return owner_process_id_;
  }

  constexpr LONG
    GetBasePriority() const noexcept
  {
    return base_priority_;
  }

  constexpr LONG
    GetDeltaPriority() const noexcept
  {
    return delta_priority_;
  }

  constexpr DWORD GetFlags() const noexcept
  {
    return flags_;
  }

private:
  DWORD usage_;
  DWORD thread_id_;
  DWORD owner_process_id_;
  LONG base_priority_;
  LONG delta_priority_;
  DWORD flags_;
};
}
