// Copyright (C) 2010-2012 Joshua Boyce.
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

class Section
{
public:
  explicit Section(Process const& process, PeFile const& pe_file, WORD number);

  Section(Section const& other);
  
  Section& operator=(Section const& other);

  Section(Section&& other) HADESMEM_NOEXCEPT;
  
  Section& operator=(Section&& other) HADESMEM_NOEXCEPT;
  
  ~Section();

  PVOID GetBase() const HADESMEM_NOEXCEPT;

  WORD GetNumber() const;

  std::string GetName() const;

  DWORD GetVirtualAddress() const;

  DWORD GetVirtualSize() const;

  DWORD GetSizeOfRawData() const;

  DWORD GetPointerToRawData() const;

  DWORD GetPointerToRelocations() const;

  DWORD GetPointerToLinenumbers() const;

  WORD GetNumberOfRelocations() const;

  WORD GetNumberOfLinenumbers() const;

  DWORD GetCharacteristics() const;

  void SetName(std::string const& Name) const;

  void SetVirtualAddress(DWORD VirtualAddress) const;

  void SetVirtualSize(DWORD VirtualSize) const;

  void SetSizeOfRawData(DWORD SizeOfRawData) const;

  void SetPointerToRawData(DWORD PointerToRawData) const;

  void SetPointerToRelocations(DWORD PointerToRelocations) const;

  void SetPointerToLinenumbers(DWORD PointerToLinenumbers) const;

  void SetNumberOfRelocations(WORD NumberOfRelocations) const;

  void SetNumberOfLinenumbers(WORD NumberOfLinenumbers) const;

  void SetCharacteristics(DWORD Characteristics) const;
  
private:
  friend class SectionIterator;

  explicit Section(Process const& process, PeFile const& pe_file, WORD number, 
    PVOID base);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool operator==(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT;

bool operator<(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT;

bool operator>(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, Section const& rhs);

std::wostream& operator<<(std::wostream& lhs, Section const& rhs);

}
