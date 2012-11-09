// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <iosfwd>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

namespace pelib
{

enum class PeFileType
{
  Image, 
  Data
};

class PeFile
{
public:
  PeFile(Process const* process, PVOID base, PeFileType type);

  PeFile(PeFile const& other) HADESMEM_NOEXCEPT;

  PeFile& operator=(PeFile const& other) HADESMEM_NOEXCEPT;
  
  PeFile(PeFile&& other) HADESMEM_NOEXCEPT;
  
  PeFile& operator=(PeFile&& other) HADESMEM_NOEXCEPT;
  
  ~PeFile();

  PVOID GetBase() const HADESMEM_NOEXCEPT;

  PeFileType GetType() const HADESMEM_NOEXCEPT;
  
private:
  Process const* process_;
  PVOID base_;
  PeFileType type_;
};

bool operator==(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

bool operator<(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

bool operator>(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, PeFile const& rhs);

std::wostream& operator<<(std::wostream& lhs, PeFile const& rhs);

PVOID RvaToVa(Process const& process, PeFile const& pe_file, DWORD rva);

}

}
