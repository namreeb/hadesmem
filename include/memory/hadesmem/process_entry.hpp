// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>

#include <windows.h>
#include <tlhelp32.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class ProcessEntry
{
public:
  ProcessEntry() 
    : id_(0), 
    threads_(0), 
    parent_(0), 
    priority_(0), 
    name_()
  { }
  
  ProcessEntry(PROCESSENTRY32 const& entry)
    : id_(entry.th32ProcessID), 
    threads_(entry.cntThreads), 
    parent_(entry.th32ParentProcessID), 
    priority_(entry.pcPriClassBase), 
    name_(entry.szExeFile)
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
