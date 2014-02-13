// Copyright (C) 2010-2014 Joshua Boyce.
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
    : id_(entry.th32ProcessID),
      threads_(entry.cntThreads),
      parent_(entry.th32ParentProcessID),
      priority_(entry.pcPriClassBase),
      name_(entry.szExeFile)
  {
  }

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  ProcessEntry(ProcessEntry const&) = default;

  ProcessEntry& operator=(ProcessEntry const&) = default;

  ProcessEntry(ProcessEntry&& other)
    : id_(other.id_),
      threads_(other.threads_),
      parent_(other.parent_),
      priority_(other.priority_),
      name_(std::move(other.name_))
  {
  }

  ProcessEntry& operator=(ProcessEntry&& other)
  {
    id_ = other.id_;
    threads_ = other.threads_;
    parent_ = other.parent_;
    priority_ = other.priority_;
    name_ = std::move(other.name_);

    return *this;
  }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  DWORD GetId() const HADESMEM_DETAIL_NOEXCEPT
  {
    return id_;
  }

  DWORD GetThreads() const HADESMEM_DETAIL_NOEXCEPT
  {
    return threads_;
  }

  DWORD GetParentId() const HADESMEM_DETAIL_NOEXCEPT
  {
    return parent_;
  }

  LONG GetPriority() const HADESMEM_DETAIL_NOEXCEPT
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
