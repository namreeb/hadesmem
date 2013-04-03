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

  void SetCharacteristics(DWORD characteristics);

  void SetTimeDateStamp(DWORD time_date_stamp);

  void SetMajorVersion(WORD major_version);

  void SetMinorVersion(WORD minor_version);

  void SetName(std::string const& name);

  void SetOrdinalBase(DWORD ordinal_base);

  void SetNumberOfFunctions(DWORD number_of_functions);

  void SetNumberOfNames(DWORD number_of_names);

  void SetAddressOfFunctions(DWORD address_of_functions);

  void SetAddressOfNames(DWORD address_of_names);

  void SetAddressOfNameOrdinals(DWORD address_of_name_ordinals);
  
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
