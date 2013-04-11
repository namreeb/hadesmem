// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <memory>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

class ProcessEntry
{
public:
  explicit ProcessEntry(PROCESSENTRY32 const& entry);

  ProcessEntry(ProcessEntry const& other);

  ProcessEntry& operator=(ProcessEntry const& other);

  ProcessEntry(ProcessEntry&& other) HADESMEM_NOEXCEPT;

  ProcessEntry& operator=(ProcessEntry&& other) HADESMEM_NOEXCEPT;

  ~ProcessEntry();

  DWORD GetId() const HADESMEM_NOEXCEPT;

  DWORD GetThreads() const HADESMEM_NOEXCEPT;

  DWORD GetParentId() const HADESMEM_NOEXCEPT;

  LONG GetPriority() const HADESMEM_NOEXCEPT;

  std::wstring GetName() const;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
