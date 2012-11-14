// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

PVOID Alloc(Process const& process, SIZE_T size);

void Free(Process const& process, LPVOID address);

class Allocator
{
public:
  Allocator(Process const* process, SIZE_T size);
  
  Allocator(Allocator&& other) HADESMEM_NOEXCEPT;
  
  Allocator& operator=(Allocator&& other) HADESMEM_NOEXCEPT;
  
  ~Allocator();
  
  void Free();
  
  PVOID GetBase() const HADESMEM_NOEXCEPT;
  
  SIZE_T GetSize() const HADESMEM_NOEXCEPT;
  
private:
  Allocator(Allocator const& other) HADESMEM_DELETED_FUNCTION;
  Allocator& operator=(Allocator const& other) HADESMEM_DELETED_FUNCTION;
  
  void FreeUnchecked() HADESMEM_NOEXCEPT;
  
  Process const* process_;
  PVOID base_;
  SIZE_T size_;
};

bool operator==(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT;

bool operator<(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT;

bool operator>(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, Allocator const& rhs);

std::wostream& operator<<(std::wostream& lhs, Allocator const& rhs);

}
