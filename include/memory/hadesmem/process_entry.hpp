// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <memory>
#include <utility>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

class ProcessEntry
{
public:
  explicit ProcessEntry(PROCESSENTRY32 const& entry)
    : id_(entry.th32ProcessID), 
    threads_(entry.cntThreads), 
    parent_(entry.th32ParentProcessID), 
    priority_(entry.pcPriClassBase), 
    name_(entry.szExeFile)
  { }

  ProcessEntry(ProcessEntry const& other)
    : id_(other.id_), 
    threads_(other.threads_), 
    parent_(other.parent_), 
    priority_(other.priority_), 
    name_(other.name_)
  { }

  ProcessEntry& operator=(ProcessEntry const& other)
  {
    id_ = other.id_;
    threads_ = other.threads_;
    priority_ = other.priority_;
    name_ = other.name_;

    return *this;
  }

  ProcessEntry(ProcessEntry&& other) HADESMEM_NOEXCEPT
    : id_(other.id_), 
    threads_(other.threads_), 
    parent_(other.parent_), 
    priority_(other.priority_), 
    name_(std::move(other.name_))
  { }

  ProcessEntry& operator=(ProcessEntry&& other) HADESMEM_NOEXCEPT
  {
    id_ = other.id_;
    threads_ = other.threads_;
    priority_ = other.priority_;
    name_ = std::move(other.name_);

    return *this;
  }

  ~ProcessEntry() HADESMEM_NOEXCEPT
  { }

  DWORD GetId() const HADESMEM_NOEXCEPT
  {
    return id_;
  }

  DWORD GetThreads() const HADESMEM_NOEXCEPT
  {
    return threads_;
  }

  DWORD GetParentId() const HADESMEM_NOEXCEPT
  {
    return parent_;
  }

  LONG GetPriority() const HADESMEM_NOEXCEPT
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
