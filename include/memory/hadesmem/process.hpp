// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <iosfwd>
#include <memory>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process
{
public:
  explicit Process(DWORD id);
  
  Process(Process const& other);
  
  Process& operator=(Process const& other);
  
  Process(Process&& other) HADESMEM_NOEXCEPT;
  
  Process& operator=(Process&& other) HADESMEM_NOEXCEPT;
  
  ~Process();
  
  DWORD GetId() const HADESMEM_NOEXCEPT;
  
  HANDLE GetHandle() const HADESMEM_NOEXCEPT;
  
  void Cleanup();
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool operator==(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT;

bool operator<(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT;

bool operator>(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(Process const& lhs, Process const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, Process const& rhs);

std::wostream& operator<<(std::wostream& lhs, Process const& rhs);

std::wstring GetPath(Process const& process);

bool IsWoW64(Process const& process);

void GetSeDebugPrivilege();

}
