// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>
#include <memory>
#include <string>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class PeFile;

// TODO: Constructor to create Export by name (optimize using binary search).

class Export
{
public:
  explicit Export(Process const& process, PeFile const& pe_file, 
    WORD ordinal);
  
  Export(Export const& other);
  
  Export& operator=(Export const& other);

  Export(Export&& other) HADESMEM_NOEXCEPT;
  
  Export& operator=(Export&& other) HADESMEM_NOEXCEPT;
  
  ~Export();

  DWORD GetRva() const;

  PVOID GetVa() const;

  std::string GetName() const;

  WORD GetOrdinal() const;

  bool ByName() const;

  bool ByOrdinal() const;

  bool IsForwarded() const;

  std::string GetForwarder() const;

  std::string GetForwarderModule() const;

  std::string GetForwarderFunction() const;

  bool IsForwardedByOrdinal() const;

  WORD GetForwarderOrdinal() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool operator==(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT;

bool operator<(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT;

bool operator>(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(Export const& lhs, Export const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, Export const& rhs);

std::wostream& operator<<(std::wostream& lhs, Export const& rhs);

}
