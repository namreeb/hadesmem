// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <string>
#include <utility>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
class ProcessEntry
{
public:
  explicit ProcessEntry(PROCESSENTRY32W const& entry)
    : id_{entry.th32ProcessID},
      threads_{entry.cntThreads},
      parent_{entry.th32ParentProcessID},
      priority_{entry.pcPriClassBase},
      name_(entry.szExeFile)
  {
  }

  DWORD GetId() const noexcept
  {
    return id_;
  }

  DWORD GetThreads() const noexcept
  {
    return threads_;
  }

  DWORD GetParentId() const noexcept
  {
    return parent_;
  }

  LONG GetPriority() const noexcept
  {
    return priority_;
  }

  std::wstring GetName() const
  {
    return name_;
  }

private:
  DWORD id_;
  DWORD threads_;
  DWORD parent_;
  LONG priority_;
  std::wstring name_;
};
}
