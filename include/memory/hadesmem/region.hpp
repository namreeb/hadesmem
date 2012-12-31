// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>
#include <memory>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class Region
{
public:
  explicit Region(Process const& process, LPCVOID address);
  
  explicit Region(Process const& process, MEMORY_BASIC_INFORMATION const& mbi);

  Region(Region const& other);

  Region& operator=(Region const& other);

  Region(Region&& other) HADESMEM_NOEXCEPT;

  Region& operator=(Region&& other) HADESMEM_NOEXCEPT;

  ~Region();
  
  PVOID GetBase() const HADESMEM_NOEXCEPT;
  
  PVOID GetAllocBase() const HADESMEM_NOEXCEPT;
  
  DWORD GetAllocProtect() const HADESMEM_NOEXCEPT;
  
  SIZE_T GetSize() const HADESMEM_NOEXCEPT;
  
  DWORD GetState() const HADESMEM_NOEXCEPT;
  
  DWORD GetProtect() const HADESMEM_NOEXCEPT;
  
  DWORD GetType() const HADESMEM_NOEXCEPT;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
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
