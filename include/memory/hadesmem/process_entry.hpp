// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <string>

#include <boost/config.hpp>

#include <windows.h>
#include <tlhelp32.h>

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

  DWORD GetId() const BOOST_NOEXCEPT
  {
    return id_;
  }

  DWORD GetThreads() const BOOST_NOEXCEPT
  {
    return threads_;
  }

  DWORD GetParentId() const BOOST_NOEXCEPT
  {
    return parent_;
  }

  LONG GetPriority() const BOOST_NOEXCEPT
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
