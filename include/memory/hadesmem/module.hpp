// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <functional>

#include <windows.h>
#include <tlhelp32.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class Module
{
public:
  explicit Module(Process const& process, HMODULE handle);
  
  explicit Module(Process const& process, std::wstring const& path);
  
  Module(Module const& other);

  Module& operator=(Module const& other);

  Module(Module&& other) HADESMEM_NOEXCEPT;

  Module& operator=(Module&& other) HADESMEM_NOEXCEPT;

  ~Module();
  
  HMODULE GetHandle() const HADESMEM_NOEXCEPT;
  
  DWORD GetSize() const HADESMEM_NOEXCEPT;
  
  std::wstring GetName() const;
  
  std::wstring GetPath() const;
  
private:
  friend class ModuleIterator;

  explicit Module(Process const& process, MODULEENTRY32 const& entry);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool operator==(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT;

bool operator<(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT;

bool operator>(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, Module const& rhs);

std::wostream& operator<<(std::wostream& lhs, Module const& rhs);

FARPROC FindProcedure(Module const& module, std::string const& name);

FARPROC FindProcedure(Module const& module, WORD ordinal);

}
