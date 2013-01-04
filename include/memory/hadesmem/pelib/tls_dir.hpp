// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>
#include <memory>
#include <vector>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class PeFile;

class TlsDir
{
public:
  explicit TlsDir(Process const& process, PeFile const& pe_file);

  TlsDir(TlsDir const& other);
  
  TlsDir& operator=(TlsDir const& other);

  TlsDir(TlsDir&& other) HADESMEM_NOEXCEPT;
  
  TlsDir& operator=(TlsDir&& other) HADESMEM_NOEXCEPT;
  
  ~TlsDir();

  PVOID GetBase() const HADESMEM_NOEXCEPT;

  DWORD_PTR GetStartAddressOfRawData() const;

  DWORD_PTR GetEndAddressOfRawData() const;

  DWORD_PTR GetAddressOfIndex() const;

  DWORD_PTR GetAddressOfCallBacks() const;

  std::vector<PIMAGE_TLS_CALLBACK> GetCallbacks() const;

  DWORD GetSizeOfZeroFill() const;

  DWORD GetCharacteristics() const;

  void SetStartAddressOfRawData(DWORD_PTR start_address_of_raw_data) const;

  void SetEndAddressOfRawData(DWORD_PTR end_address_of_raw_data) const;

  void SetAddressOfIndex(DWORD_PTR address_of_index) const;

  void SetAddressOfCallBacks(DWORD_PTR address_of_callbacks) const;

  // TODO: SetCallbacks function

  void SetSizeOfZeroFill(DWORD size_of_zero_fill) const;

  void SetCharacteristics(DWORD characteristics) const;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

bool operator==(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT;

bool operator!=(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT;

bool operator<(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT;

bool operator<=(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT;

bool operator>(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT;

bool operator>=(TlsDir const& lhs, TlsDir const& rhs) HADESMEM_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, TlsDir const& rhs);

std::wostream& operator<<(std::wostream& lhs, TlsDir const& rhs);

}
