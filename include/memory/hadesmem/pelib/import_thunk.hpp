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

class ImportThunk
{
public:
  explicit ImportThunk(Process const& process, PeFile const& pe_file, 
    PIMAGE_THUNK_DATA thunk);
  
  ImportThunk(ImportThunk const& other);
  
  ImportThunk& operator=(ImportThunk const& other);

  ImportThunk(ImportThunk&& other) HADESMEM_NOEXCEPT;
  
  ImportThunk& operator=(ImportThunk&& other) HADESMEM_NOEXCEPT;
  
  ~ImportThunk();

  PVOID GetBase() const HADESMEM_NOEXCEPT;

  DWORD_PTR GetAddressOfData() const;
  
  DWORD_PTR GetOrdinalRaw() const;

  bool ByOrdinal() const;

  WORD GetOrdinal() const;

  DWORD_PTR GetFunction() const;

  WORD GetHint() const;

  std::string GetName() const;

  void SetAddressOfData(DWORD_PTR address_of_data);

  void SetOrdinalRaw(DWORD_PTR ordinal_raw);

  // Todo: SetOrdinal function

  void SetFunction(DWORD_PTR function);

  void SetHint(WORD hint);

  // TODO: SetName function

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool operator==(ImportThunk const& lhs, ImportThunk const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(ImportThunk const& lhs, ImportThunk const& rhs) HADESMEM_NOEXCEPT;

bool operator<(ImportThunk const& lhs, ImportThunk const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(ImportThunk const& lhs, ImportThunk const& rhs) HADESMEM_NOEXCEPT;

bool operator>(ImportThunk const& lhs, ImportThunk const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(ImportThunk const& lhs, ImportThunk const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, ImportThunk const& rhs);

std::wostream& operator<<(std::wostream& lhs, ImportThunk const& rhs);

}
