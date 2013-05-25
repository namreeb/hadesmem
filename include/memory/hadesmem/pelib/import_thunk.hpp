// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <cstddef>
#include <ostream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/assert.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/read.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/write.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/nt_headers.hpp>

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

namespace hadesmem
{

class ImportThunk
{
public:
  explicit ImportThunk(Process const& process, PeFile const& pe_file, 
    PIMAGE_THUNK_DATA thunk)
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(reinterpret_cast<PBYTE>(thunk))
  { }
  
  ImportThunk(ImportThunk const& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    base_(other.base_)
  { }
  
  ImportThunk& operator=(ImportThunk const& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    base_ = other.base_;

    return *this;
  }

  ImportThunk(ImportThunk&& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    base_(other.base_)
  { }
  
  ImportThunk& operator=(ImportThunk&& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    base_ = other.base_;

    return *this;
  }
  
  ~ImportThunk() HADESMEM_NOEXCEPT
  { }

  PVOID GetBase() const HADESMEM_NOEXCEPT
  {
    return base_;
  }
  
  DWORD_PTR GetAddressOfData() const
  {
    return Read<DWORD_PTR>(*process_, base_ + offsetof(IMAGE_THUNK_DATA, 
      u1.AddressOfData));
  }

  void SetAddressOfData(DWORD_PTR address_of_data)
  {
    return Write(*process_, base_ + offsetof(IMAGE_THUNK_DATA, 
      u1.AddressOfData), address_of_data);
  }

  DWORD_PTR GetOrdinalRaw() const
  {
    return Read<DWORD_PTR>(*process_, base_ + offsetof(IMAGE_THUNK_DATA, 
      u1.Ordinal));
  }

  void SetOrdinalRaw(DWORD_PTR ordinal_raw)
  {
    return Write(*process_, base_ + offsetof(IMAGE_THUNK_DATA, 
      u1.Ordinal), ordinal_raw);
  }

  bool ByOrdinal() const
  {
    return IMAGE_SNAP_BY_ORDINAL(GetOrdinalRaw());
  }

  WORD GetOrdinal() const
  {
    return IMAGE_ORDINAL(GetOrdinalRaw());
  }

  // Todo: SetOrdinal function

  DWORD_PTR GetFunction() const
  {
    return Read<DWORD_PTR>(*process_, base_ + offsetof(IMAGE_THUNK_DATA, 
      u1.Function));
  }

  void SetFunction(DWORD_PTR function)
  {
    return Write(*process_, base_ + offsetof(IMAGE_THUNK_DATA, 
      u1.Function), function);
  }

  WORD GetHint() const
  {
    PBYTE const name_import = static_cast<PBYTE>(RvaToVa(*process_, *pe_file_, 
      static_cast<DWORD>(GetAddressOfData())));
    return Read<WORD>(*process_, name_import + offsetof(
      IMAGE_IMPORT_BY_NAME, Hint));
  }

  void SetHint(WORD hint)
  {
    PBYTE const name_import = static_cast<PBYTE>(RvaToVa(*process_, *pe_file_, 
      static_cast<DWORD>(GetAddressOfData())));
    return Write(*process_, name_import + offsetof(
      IMAGE_IMPORT_BY_NAME, Hint), hint);
  }

  std::string GetName() const
  {
    PBYTE const name_import = static_cast<PBYTE>(RvaToVa(*process_, *pe_file_, 
      static_cast<DWORD>(GetAddressOfData())));
    return ReadString<char>(*process_, name_import + offsetof(
      IMAGE_IMPORT_BY_NAME, Name));
  }

  // TODO: SetName function

private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

inline bool operator==(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(ImportThunk const& lhs, ImportThunk const& rhs) 
  HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, ImportThunk const& rhs)
{
  return (lhs << rhs.GetBase());
}

inline std::wostream& operator<<(std::wostream& lhs, ImportThunk const& rhs)
{
  return (lhs << rhs.GetBase());
}

}

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif