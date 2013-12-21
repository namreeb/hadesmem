// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <memory>
#include <iosfwd>
#include <ostream>
#include <string>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

// TODO: Support setting and writing back ImportThunk. (For IAT hooking.)

namespace hadesmem
{

class ImportThunk
{
public:
  explicit ImportThunk(Process const& process,
                       PeFile const& pe_file,
                       PIMAGE_THUNK_DATA thunk)
    : process_(&process),
      pe_file_(&pe_file),
      base_(reinterpret_cast<PBYTE>(thunk))
  {
  }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  DWORD_PTR GetAddressOfData() const
  {
    return Read<DWORD_PTR>(
      *process_, base_ + offsetof(IMAGE_THUNK_DATA, u1.AddressOfData));
  }

  void SetAddressOfData(DWORD_PTR address_of_data)
  {
    return Write(*process_,
                 base_ + offsetof(IMAGE_THUNK_DATA, u1.AddressOfData),
                 address_of_data);
  }

  DWORD_PTR GetOrdinalRaw() const
  {
    return Read<DWORD_PTR>(*process_,
                           base_ + offsetof(IMAGE_THUNK_DATA, u1.Ordinal));
  }

  void SetOrdinalRaw(DWORD_PTR ordinal_raw)
  {
    return Write(
      *process_, base_ + offsetof(IMAGE_THUNK_DATA, u1.Ordinal), ordinal_raw);
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
    return Read<DWORD_PTR>(*process_,
                           base_ + offsetof(IMAGE_THUNK_DATA, u1.Function));
  }

  void SetFunction(DWORD_PTR function)
  {
    return Write(
      *process_, base_ + offsetof(IMAGE_THUNK_DATA, u1.Function), function);
  }

  WORD GetHint() const
  {
    PBYTE const name_import = static_cast<PBYTE>(
      RvaToVa(*process_, *pe_file_, static_cast<DWORD>(GetAddressOfData())));
    if (!name_import)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
        << ErrorString("Invalid import name and hint."));
    }
    return Read<WORD>(*process_,
                      name_import + offsetof(IMAGE_IMPORT_BY_NAME, Hint));
  }

  void SetHint(WORD hint)
  {
    PBYTE const name_import = static_cast<PBYTE>(
      RvaToVa(*process_, *pe_file_, static_cast<DWORD>(GetAddressOfData())));
    return Write(
      *process_, name_import + offsetof(IMAGE_IMPORT_BY_NAME, Hint), hint);
  }

  std::string GetName() const
  {
    PBYTE const name_import = static_cast<PBYTE>(
      RvaToVa(*process_, *pe_file_, static_cast<DWORD>(GetAddressOfData())));
    if (!name_import)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
        << ErrorString("Invalid import name and hint."));
    }
    return ReadString<char>(*process_,
                            name_import + offsetof(IMAGE_IMPORT_BY_NAME, Name));
  }

  // TODO: SetName function

private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

inline bool operator==(ImportThunk const& lhs, ImportThunk const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(ImportThunk const& lhs, ImportThunk const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(ImportThunk const& lhs, ImportThunk const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(ImportThunk const& lhs, ImportThunk const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(ImportThunk const& lhs, ImportThunk const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(ImportThunk const& lhs, ImportThunk const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, ImportThunk const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, ImportThunk const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}
}

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif