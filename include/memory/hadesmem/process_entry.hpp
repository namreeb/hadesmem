// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <string>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/config.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <tlhelp32.h>

namespace hadesmem
{

struct ProcessEntry
{
  ProcessEntry() 
    : id(0), 
    threads(0), 
    parent(0), 
    priority(0), 
    name()
  { }
  
  ProcessEntry(PROCESSENTRY32 const& entry)
    : id(entry.th32ProcessID), 
    threads(entry.cntThreads), 
    parent(entry.th32ParentProcessID), 
    priority(entry.pcPriClassBase), 
    name(entry.szExeFile)
  { }
  
  DWORD id;
  DWORD threads;
  DWORD parent;
  DWORD priority;
  std::wstring name;
};

}
