// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{

class ExportDir
{
public:
  explicit ExportDir(Process const& process, PeFile const& pe_file)
    : process_{&process}, pe_file_{&pe_file}
  {
    NtHeaders nt_headers{process, pe_file};
    DWORD const export_dir_rva =
      nt_headers.GetDataDirectoryVirtualAddress(PeDataDir::Export);
    // Windows will load images which don't specify a size for the export
    // directory.
    if (!export_dir_rva)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Export directory is invalid."});
    }

    base_ =
      static_cast<std::uint8_t*>(RvaToVa(process, pe_file, export_dir_rva));
    if (!base_)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Export directory is invalid."});
    }

    UpdateRead();
  }

  explicit ExportDir(Process&& process, PeFile const& pe_file) = delete;

  explicit ExportDir(Process const& process, PeFile&& pe_file) = delete;

  explicit ExportDir(Process&& process, PeFile&& pe_file) = delete;

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  void UpdateRead()
  {
    data_ = Read<IMAGE_EXPORT_DIRECTORY>(*process_, base_);
  }

  void UpdateWrite()
  {
    Write(*process_, base_, data_);
  }

  DWORD GetCharacteristics() const
  {
    return data_.Characteristics;
  }

  DWORD GetTimeDateStamp() const
  {
    return data_.TimeDateStamp;
  }

  WORD GetMajorVersion() const
  {
    return data_.MajorVersion;
  }

  WORD GetMinorVersion() const
  {
    return data_.MinorVersion;
  }

  DWORD GetNameRaw() const
  {
    return data_.Name;
  }

  std::string GetName() const
  {
    DWORD const name_rva = GetNameRaw();
    if (!name_rva)
    {
      return {};
    }

    auto const name_va = RvaToVa(*process_, *pe_file_, name_rva);
    if (!name_va)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Name VA is invalid."});
    }

    return detail::CheckedReadString<char>(*process_, *pe_file_, name_va);
  }

  DWORD GetOrdinalBase() const
  {
    return data_.Base;
  }

  DWORD GetNumberOfFunctions() const
  {
    return data_.NumberOfFunctions;
  }

  DWORD GetNumberOfNames() const
  {
    return data_.NumberOfNames;
  }

  DWORD GetAddressOfFunctions() const
  {
    return data_.AddressOfFunctions;
  }

  DWORD GetAddressOfNames() const
  {
    return data_.AddressOfNames;
  }

  DWORD GetAddressOfNameOrdinals() const
  {
    return data_.AddressOfNameOrdinals;
  }

  void SetCharacteristics(DWORD characteristics)
  {
    data_.Characteristics = characteristics;
  }

  void SetTimeDateStamp(DWORD time_date_stamp)
  {
    data_.TimeDateStamp = time_date_stamp;
  }

  void SetMajorVersion(WORD major_version)
  {
    data_.MajorVersion = major_version;
  }

  void SetMinorVersion(WORD minor_version)
  {
    data_.MinorVersion = minor_version;
  }

  void SetName(std::string const& name)
  {
    DWORD const name_rva = GetNameRaw();

    if (!name_rva)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Export dir has no name. Cannot overwrite."});
    }

    std::string const current_name =
      ReadString<char>(*process_, RvaToVa(*process_, *pe_file_, name_rva));

    if (name.size() > current_name.size())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"Cannot overwrite name with longer string."});
    }

    WriteString(*process_, RvaToVa(*process_, *pe_file_, name_rva), name);
  }

  void SetOrdinalBase(DWORD base)
  {
    data_.Base = base;
  }

  void SetNumberOfFunctions(DWORD number_of_functions)
  {
    data_.NumberOfFunctions = number_of_functions;
  }

  void SetNumberOfNames(DWORD number_of_names)
  {
    data_.NumberOfNames = number_of_names;
  }

  void SetAddressOfFunctions(DWORD address_of_functions)
  {
    data_.AddressOfFunctions = address_of_functions;
  }

  void SetAddressOfNames(DWORD address_of_names)
  {
    data_.AddressOfNames = address_of_names;
  }

  void SetAddressOfNameOrdinals(DWORD address_of_name_ordinals)
  {
    data_.AddressOfNameOrdinals = address_of_name_ordinals;
  }

private:
  Process const* process_{};
  PeFile const* pe_file_{};
  PBYTE base_{};
  IMAGE_EXPORT_DIRECTORY data_ = IMAGE_EXPORT_DIRECTORY{};
};

inline bool operator==(ExportDir const& lhs,
                       ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(ExportDir const& lhs,
                       ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(ExportDir const& lhs,
                      ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(ExportDir const& lhs,
                       ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(ExportDir const& lhs,
                      ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(ExportDir const& lhs,
                       ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, ExportDir const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, ExportDir const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}
}
