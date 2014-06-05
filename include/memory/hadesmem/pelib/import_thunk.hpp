// Copyright (C) 2010-2014 Joshua Boyce.
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

namespace hadesmem
{

class ImportThunk
{
public:
  explicit ImportThunk(Process const& process,
                       PeFile const& pe_file,
                       PIMAGE_THUNK_DATA thunk)
    : process_{&process},
      pe_file_{&pe_file},
      base_{reinterpret_cast<std::uint8_t*>(thunk)}
  {
    UpdateRead();
  }

  explicit ImportThunk(Process&& process,
                       PeFile const& pe_file,
                       PIMAGE_THUNK_DATA thunk) = delete;

  explicit ImportThunk(Process const& process,
                       PeFile&& pe_file,
                       PIMAGE_THUNK_DATA thunk) = delete;

  explicit ImportThunk(Process&& process,
                       PeFile&& pe_file,
                       PIMAGE_THUNK_DATA thunk) = delete;

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  void UpdateRead()
  {
    data_ = Read<IMAGE_THUNK_DATA>(*process_, base_);
  }

  void UpdateWrite()
  {
    Write(*process_, base_, data_);
  }

  DWORD_PTR GetAddressOfData() const
  {
    return data_.u1.AddressOfData;
  }

  DWORD_PTR GetOrdinalRaw() const
  {
    return data_.u1.Ordinal;
  }

  bool ByOrdinal() const
  {
    return IMAGE_SNAP_BY_ORDINAL(GetOrdinalRaw());
  }

  WORD GetOrdinal() const
  {
    return IMAGE_ORDINAL(GetOrdinalRaw());
  }

  DWORD_PTR GetFunction() const
  {
    return data_.u1.Function;
  }

  WORD GetHint() const
  {
    auto const name_import = static_cast<std::uint8_t*>(
      RvaToVa(*process_, *pe_file_, static_cast<DWORD>(GetAddressOfData())));
    if (!name_import)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Invalid import name and hint."});
    }
    return Read<WORD>(*process_,
                      name_import + offsetof(IMAGE_IMPORT_BY_NAME, Hint));
  }

  std::string GetName() const
  {
    auto const name_import = static_cast<std::uint8_t*>(
      RvaToVa(*process_, *pe_file_, static_cast<DWORD>(GetAddressOfData())));
    if (!name_import)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Invalid import name and hint."});
    }
    return detail::CheckedReadString<char>(
      *process_, *pe_file_, name_import + offsetof(IMAGE_IMPORT_BY_NAME, Name));
  }

  void SetAddressOfData(DWORD_PTR address_of_data)
  {
    data_.u1.AddressOfData = address_of_data;
  }

  void SetOrdinalRaw(DWORD_PTR ordinal_raw)
  {
    data_.u1.Ordinal = ordinal_raw;
  }

  void SetFunction(DWORD_PTR function)
  {
    data_.u1.Function = function;
  }

  void SetHint(WORD hint)
  {
    std::uint8_t* const name_import = static_cast<PBYTE>(
      RvaToVa(*process_, *pe_file_, static_cast<DWORD>(GetAddressOfData())));
    return Write(
      *process_, name_import + offsetof(IMAGE_IMPORT_BY_NAME, Hint), hint);
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
  IMAGE_THUNK_DATA data_ = IMAGE_THUNK_DATA{};
};

inline bool operator==(ImportThunk const& lhs,
                       ImportThunk const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(ImportThunk const& lhs,
                       ImportThunk const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(ImportThunk const& lhs,
                      ImportThunk const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(ImportThunk const& lhs,
                       ImportThunk const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(ImportThunk const& lhs,
                      ImportThunk const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(ImportThunk const& lhs,
                       ImportThunk const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, ImportThunk const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, ImportThunk const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}
}
