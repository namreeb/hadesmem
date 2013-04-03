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

  WORD GetNumber() const HADESMEM_NOEXCEPT;

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

  void SetName(std::string const& Name);

  void SetVirtualAddress(DWORD VirtualAddress);

  void SetVirtualSize(DWORD VirtualSize);

  void SetSizeOfRawData(DWORD SizeOfRawData);

  void SetPointerToRawData(DWORD PointerToRawData);

  void SetPointerToRelocations(DWORD PointerToRelocations);

  void SetPointerToLinenumbers(DWORD PointerToLinenumbers);

  void SetNumberOfRelocations(WORD NumberOfRelocations);

  void SetNumberOfLinenumbers(WORD NumberOfLinenumbers);

  void SetCharacteristics(DWORD Characteristics);
  
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
