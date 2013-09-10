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
#include <hadesmem/pelib/nt_headers.hpp>

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

namespace hadesmem
{

class ExportDir
{
public:
  explicit ExportDir(Process const& process, PeFile const& pe_file)
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(nullptr)
  {
    NtHeaders nt_headers(process, pe_file);
    DWORD const export_dir_rva = nt_headers.GetDataDirectoryVirtualAddress(
      PeDataDir::Export);
    // Windows will load images which don't specify a size for the export 
    // directory.
    if (!export_dir_rva)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
        ErrorString("Export directory is invalid."));
    }

    base_ = static_cast<PBYTE>(RvaToVa(process, pe_file, export_dir_rva));
  }

  ExportDir(ExportDir const& other) HADESMEM_DETAIL_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    base_(other.base_)
  { }
  
  ExportDir& operator=(ExportDir const& other) HADESMEM_DETAIL_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    base_ = other.base_;

    return *this;
  }

  ExportDir(ExportDir&& other) HADESMEM_DETAIL_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    base_(other.base_)
  { }
  
  ExportDir& operator=(ExportDir&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    base_ = other.base_;

    return *this;
  }
  
  ~ExportDir() HADESMEM_DETAIL_NOEXCEPT
  { }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }
  
  DWORD GetCharacteristics() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(
      IMAGE_EXPORT_DIRECTORY, Characteristics));
  }

  DWORD GetTimeDateStamp() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(
      IMAGE_EXPORT_DIRECTORY, TimeDateStamp));
  }

  WORD GetMajorVersion() const
  {
    return Read<WORD>(*process_, base_ + offsetof(
      IMAGE_EXPORT_DIRECTORY, MajorVersion));
  }

  WORD GetMinorVersion() const
  {
    return Read<WORD>(*process_, base_ + offsetof(
      IMAGE_EXPORT_DIRECTORY, MinorVersion));
  }

  std::string GetName() const
  {
    DWORD const name_rva = Read<DWORD>(*process_, base_ + 
      offsetof(IMAGE_EXPORT_DIRECTORY, Name));

    if (!name_rva)
    {
      return std::string();
    }

    PVOID name_va = RvaToVa(*process_, *pe_file_, name_rva);
    return ReadString<char>(*process_, name_va);
  }

  DWORD GetOrdinalBase() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(
      IMAGE_EXPORT_DIRECTORY, Base));
  }

  DWORD GetNumberOfFunctions() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(
      IMAGE_EXPORT_DIRECTORY, NumberOfFunctions));
  }

  DWORD GetNumberOfNames() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(
      IMAGE_EXPORT_DIRECTORY, NumberOfNames));
  }

  DWORD GetAddressOfFunctions() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(
      IMAGE_EXPORT_DIRECTORY, AddressOfFunctions));
  }

  DWORD GetAddressOfNames() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(
      IMAGE_EXPORT_DIRECTORY, AddressOfNames));
  }

  DWORD GetAddressOfNameOrdinals() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(
      IMAGE_EXPORT_DIRECTORY, AddressOfNameOrdinals));
  }

  void SetCharacteristics(DWORD characteristics)
  {
    Write(*process_, base_ + offsetof(IMAGE_EXPORT_DIRECTORY, 
      Characteristics), characteristics);
  }

  void SetTimeDateStamp(DWORD time_date_stamp)
  {
    Write(*process_, base_ + offsetof(IMAGE_EXPORT_DIRECTORY, 
      TimeDateStamp), time_date_stamp);
  }

  void SetMajorVersion(WORD major_version)
  {
    Write(*process_, base_ + offsetof(IMAGE_EXPORT_DIRECTORY, 
      MajorVersion), major_version);
  }

  void SetMinorVersion(WORD minor_version)
  {
    Write(*process_, base_ + offsetof(IMAGE_EXPORT_DIRECTORY, 
      MinorVersion), minor_version);
  }

  void SetName(std::string const& name)
  {
    DWORD const name_rva = Read<DWORD>(*process_, base_ + 
      offsetof(IMAGE_EXPORT_DIRECTORY, Name));

    if (!name_rva)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
        ErrorString("Export dir has no name. Cannot overwrite."));
    }

    std::string const current_name = ReadString<char>(*process_, 
      RvaToVa(*process_, *pe_file_, name_rva));

    // TODO: Support allocating space for a new name rather than just 
    // overwriting the existing one.
    if (name.size() > current_name.size())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
        ErrorString("Cannot overwrite name with longer string."));
    }

    WriteString(*process_, RvaToVa(*process_, *pe_file_, 
      name_rva), name);
  }

  void SetOrdinalBase(DWORD base)
  {
    Write(*process_, base_ + offsetof(IMAGE_EXPORT_DIRECTORY, 
      Base), base);
  }

  void SetNumberOfFunctions(DWORD number_of_functions)
  {
    Write(*process_, base_ + offsetof(IMAGE_EXPORT_DIRECTORY, 
      NumberOfFunctions), number_of_functions);
  }

  void SetNumberOfNames(DWORD number_of_names)
  {
    Write(*process_, base_ + offsetof(IMAGE_EXPORT_DIRECTORY, 
      NumberOfNames), number_of_names);
  }

  void SetAddressOfFunctions(DWORD address_of_functions)
  {
    Write(*process_, base_ + offsetof(IMAGE_EXPORT_DIRECTORY, 
      AddressOfFunctions), address_of_functions);
  }

  void SetAddressOfNames(DWORD address_of_names)
  {
    Write(*process_, base_ + offsetof(IMAGE_EXPORT_DIRECTORY, 
      AddressOfNames), address_of_names);
  }

  void SetAddressOfNameOrdinals(DWORD address_of_name_ordinals)
  {
    Write(*process_, base_ + offsetof(IMAGE_EXPORT_DIRECTORY, 
      AddressOfNameOrdinals), address_of_name_ordinals);
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

inline bool operator==(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, ExportDir const& rhs)
{
  std::locale old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, ExportDir const& rhs)
{
  std::locale old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

}

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif
