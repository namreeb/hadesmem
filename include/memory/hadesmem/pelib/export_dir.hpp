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

class ExportDir
{
public:
  explicit ExportDir(Process const& process, PeFile const& pe_file);

  ExportDir(ExportDir const& other);
  
  ExportDir& operator=(ExportDir const& other);

  ExportDir(ExportDir&& other) HADESMEM_NOEXCEPT;
  
  ExportDir& operator=(ExportDir&& other) HADESMEM_NOEXCEPT;
  
  ~ExportDir();

  PVOID GetBase() const HADESMEM_NOEXCEPT;

  DWORD GetCharacteristics() const;

  DWORD GetTimeDateStamp() const;

  WORD GetMajorVersion() const;

  WORD GetMinorVersion() const;

  std::string GetName() const;

  DWORD GetOrdinalBase() const;

  DWORD GetNumberOfFunctions() const;

  DWORD GetNumberOfNames() const;

  DWORD GetAddressOfFunctions() const;

  DWORD GetAddressOfNames() const;

  DWORD GetAddressOfNameOrdinals() const;

  void SetCharacteristics(DWORD characteristics) const;

  void SetTimeDateStamp(DWORD time_date_stamp) const;

  void SetMajorVersion(WORD major_version) const;

  void SetMinorVersion(WORD minor_version) const;

  void SetName(std::string const& name) const;

  void SetOrdinalBase(DWORD ordinal_base) const;

  void SetNumberOfFunctions(DWORD number_of_functions) const;

  void SetNumberOfNames(DWORD number_of_names) const;

  void SetAddressOfFunctions(DWORD address_of_functions) const;

  void SetAddressOfNames(DWORD address_of_names) const;

  void SetAddressOfNameOrdinals(DWORD address_of_name_ordinals) const;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool operator==(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT;

bool operator<(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT;

bool operator>(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, ExportDir const& rhs);

std::wostream& operator<<(std::wostream& lhs, ExportDir const& rhs);

}
