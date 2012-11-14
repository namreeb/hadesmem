// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class Region
{
public:
  Region(Process const* process, LPCVOID address);
  
  Region(Process const* process, MEMORY_BASIC_INFORMATION const& mbi) 
    HADESMEM_NOEXCEPT;
  
  PVOID GetBase() const HADESMEM_NOEXCEPT;
  
  PVOID GetAllocBase() const HADESMEM_NOEXCEPT;
  
  DWORD GetAllocProtect() const HADESMEM_NOEXCEPT;
  
  SIZE_T GetSize() const HADESMEM_NOEXCEPT;
  
  DWORD GetState() const HADESMEM_NOEXCEPT;
  
  DWORD GetProtect() const HADESMEM_NOEXCEPT;
  
  DWORD GetType() const HADESMEM_NOEXCEPT;
  
private:
  Process const* process_;
  MEMORY_BASIC_INFORMATION mbi_;
};

bool operator==(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT;

bool operator<(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT;

bool operator>(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, Region const& rhs);

std::wostream& operator<<(std::wostream& lhs, Region const& rhs);

}
