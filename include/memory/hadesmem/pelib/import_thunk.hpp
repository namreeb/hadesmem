// Copyright (C) 2010-2015 Joshua Boyce
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
                       void* thunk)
    : process_{&process},
      pe_file_{&pe_file},
      base_{static_cast<std::uint8_t*>(thunk)}
  {
    UpdateRead();
  }

  explicit ImportThunk(Process const&& process,
                       PeFile const& pe_file,
                       void* thunk) = delete;

  explicit ImportThunk(Process const& process,
                       PeFile&& pe_file,
                       void* thunk) = delete;

  explicit ImportThunk(Process const&& process,
                       PeFile&& pe_file,
                       void* thunk) = delete;

  void* GetBase() const noexcept
  {
    return base_;
  }

  void UpdateRead()
  {
    if (pe_file_->Is64())
    {
      data_64_ = Read<IMAGE_THUNK_DATA64>(*process_, base_);
    }
    else
    {
      data_32_ = Read<IMAGE_THUNK_DATA32>(*process_, base_);
    }
  }

  void UpdateWrite()
  {
    if (pe_file_->Is64())
    {
      Write(*process_, base_, data_64_);
    }
    else
    {
      Write(*process_, base_, data_32_);
    }
  }

  ULONGLONG GetAddressOfData() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.u1.AddressOfData;
    }
    else
    {
      return data_32_.u1.AddressOfData;
    }
  }

  ULONGLONG GetOrdinalRaw() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.u1.Ordinal;
    }
    else
    {
      return data_32_.u1.Ordinal;
    }
  }

  bool ByOrdinal() const
  {
    if (pe_file_->Is64())
    {
      return IMAGE_SNAP_BY_ORDINAL64(GetOrdinalRaw());
    }
    else
    {
      return IMAGE_SNAP_BY_ORDINAL32(GetOrdinalRaw());
    }
  }

  WORD GetOrdinal() const
  {
    if (pe_file_->Is64())
    {
      return IMAGE_ORDINAL64(GetOrdinalRaw());
    }
    else
    {
      return IMAGE_ORDINAL32(GetOrdinalRaw());
    }
  }

  ULONGLONG GetFunction() const
  {
    if (pe_file_->Is64())
    {
      return data_64_.u1.Function;
    }
    else
    {
      return data_32_.u1.Function;
    }
  }

  ULONGLONG* GetFunctionPtr()
  {
    if (pe_file_->Is64())
    {
      return reinterpret_cast<ULONGLONG*>(
        base_ + offsetof(IMAGE_THUNK_DATA64, u1.Function));
    }
    else
    {
      return reinterpret_cast<ULONGLONG*>(
        base_ + offsetof(IMAGE_THUNK_DATA32, u1.Function));
    }
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

  void SetAddressOfData(ULONGLONG address_of_data)
  {
    if (pe_file_->Is64())
    {
      data_64_.u1.AddressOfData = address_of_data;
    }
    else
    {
      data_32_.u1.AddressOfData = static_cast<DWORD>(address_of_data);
    }
  }

  void SetOrdinalRaw(ULONGLONG ordinal_raw)
  {
    if (pe_file_->Is64())
    {
      data_64_.u1.Ordinal = ordinal_raw;
    }
    else
    {
      data_32_.u1.Ordinal = static_cast<DWORD>(ordinal_raw);
    }
  }

  // TODO: Add SetOrdinal.

  // TODO: Add SetName.

  void SetFunction(ULONGLONG function)
  {
    if (pe_file_->Is64())
    {
      data_64_.u1.Function = function;
    }
    else
    {
      data_32_.u1.Function = static_cast<DWORD>(function);
    }
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
  IMAGE_THUNK_DATA32 data_32_ = IMAGE_THUNK_DATA32{};
  IMAGE_THUNK_DATA64 data_64_ = IMAGE_THUNK_DATA64{};
};

inline bool operator==(ImportThunk const& lhs,
                       ImportThunk const& rhs) noexcept
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(ImportThunk const& lhs,
                       ImportThunk const& rhs) noexcept
{
  return !(lhs == rhs);
}

inline bool operator<(ImportThunk const& lhs,
                      ImportThunk const& rhs) noexcept
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(ImportThunk const& lhs,
                       ImportThunk const& rhs) noexcept
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(ImportThunk const& lhs,
                      ImportThunk const& rhs) noexcept
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(ImportThunk const& lhs,
                       ImportThunk const& rhs) noexcept
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
