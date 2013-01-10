// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>
#include <memory>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

enum class PeFileType
{
  Image, 
  Data
};

class PeFile
{
public:
  explicit PeFile(Process const& process, PVOID address, PeFileType type);

  PeFile(PeFile const& other);
  
  PeFile& operator=(PeFile const& other);

  PeFile(PeFile&& other) HADESMEM_NOEXCEPT;
  
  PeFile& operator=(PeFile&& other) HADESMEM_NOEXCEPT;
  
  ~PeFile();

  PVOID GetBase() const HADESMEM_NOEXCEPT;

  PeFileType GetType() const HADESMEM_NOEXCEPT;

  PVOID RvaToVa(DWORD rva) const;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool operator==(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

bool operator<(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

bool operator>(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, PeFile const& rhs);

std::wostream& operator<<(std::wostream& lhs, PeFile const& rhs);

PVOID RvaToVa(Process const& process, PeFile const& pefile, DWORD rva);

}
