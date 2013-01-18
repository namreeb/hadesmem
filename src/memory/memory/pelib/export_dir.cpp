// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/export_dir.hpp"

#include <cstddef>
#include <ostream>
#include <utility>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <winnt.h>

#include "hadesmem/read.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/pelib/pe_file.hpp"
#include "hadesmem/pelib/nt_headers.hpp"

// TODO: Fix the code so this hack can be removed.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic ignored "-Wextended-offsetof"
#endif

namespace hadesmem
{

struct ExportDir::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file)
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(nullptr)
  {
    NtHeaders nt_headers(process, pe_file);
    DWORD const export_dir_rva = nt_headers.GetDataDirectoryVirtualAddress(
      PeDataDir::Export);
    DWORD const export_dir_size = nt_headers.GetDataDirectorySize(
      PeDataDir::Export);
    if (!export_dir_rva || !export_dir_size)
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Export directory is invalid."));
    }
    base_ = static_cast<PBYTE>(RvaToVa(process, pe_file, export_dir_rva));
  }

  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

ExportDir::ExportDir(Process const& process, PeFile const& pe_file)
  : impl_(new Impl(process, pe_file))
{ }

ExportDir::ExportDir(ExportDir const& other)
  : impl_(new Impl(*other.impl_))
{ }

ExportDir& ExportDir::operator=(ExportDir const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

ExportDir::ExportDir(ExportDir&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ExportDir& ExportDir::operator=(ExportDir&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

ExportDir::~ExportDir()
{ }

PVOID ExportDir::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

DWORD ExportDir::GetCharacteristics() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_EXPORT_DIRECTORY, Characteristics));
}

DWORD ExportDir::GetTimeDateStamp() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_EXPORT_DIRECTORY, TimeDateStamp));
}

WORD ExportDir::GetMajorVersion() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_EXPORT_DIRECTORY, MajorVersion));
}

WORD ExportDir::GetMinorVersion() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_EXPORT_DIRECTORY, MinorVersion));
}

std::string ExportDir::GetName() const
{
  DWORD const name_rva = Read<DWORD>(*impl_->process_, impl_->base_ + 
    FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, Name));

  if (!name_rva)
  {
    return std::string();
  }

  PVOID name_va = RvaToVa(*impl_->process_, *impl_->pe_file_, name_rva);
  return ReadString<char>(*impl_->process_, name_va);
}

DWORD ExportDir::GetOrdinalBase() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_EXPORT_DIRECTORY, Base));
}

DWORD ExportDir::GetNumberOfFunctions() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_EXPORT_DIRECTORY, NumberOfFunctions));
}

DWORD ExportDir::GetNumberOfNames() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_EXPORT_DIRECTORY, NumberOfNames));
}

DWORD ExportDir::GetAddressOfFunctions() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_EXPORT_DIRECTORY, AddressOfFunctions));
}

DWORD ExportDir::GetAddressOfNames() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_EXPORT_DIRECTORY, AddressOfNames));
}

DWORD ExportDir::GetAddressOfNameOrdinals() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + FIELD_OFFSET(
    IMAGE_EXPORT_DIRECTORY, AddressOfNameOrdinals));
}

void ExportDir::SetCharacteristics(DWORD characteristics) const
{
  Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, 
    Characteristics), characteristics);
}

void ExportDir::SetTimeDateStamp(DWORD time_date_stamp) const
{
  Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, 
    TimeDateStamp), time_date_stamp);
}

void ExportDir::SetMajorVersion(WORD major_version) const
{
  Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, 
    MajorVersion), major_version);
}

void ExportDir::SetMinorVersion(WORD minor_version) const
{
  Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, 
    MinorVersion), minor_version);
}

void ExportDir::SetName(std::string const& name) const
{
  DWORD const name_rva = Read<DWORD>(*impl_->process_, impl_->base_ + 
    FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, Name));

  if (!name_rva)
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Export dir has no name. Cannot overwrite."));
  }

  std::string const current_name = ReadString<char>(*impl_->process_, 
    RvaToVa(*impl_->process_, *impl_->pe_file_, name_rva));

  // FIXME: Support allocating space for a new name rather than just 
  // overwriting the existing one.
  if (name.size() > current_name.size())
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Cannot overwrite name with longer string."));
  }

  WriteString(*impl_->process_, RvaToVa(*impl_->process_, *impl_->pe_file_, 
    name_rva), name);
}

void ExportDir::SetOrdinalBase(DWORD base) const
{
  Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, 
    Base), base);
}

void ExportDir::SetNumberOfFunctions(DWORD number_of_functions) const
{
  Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, 
    NumberOfFunctions), number_of_functions);
}

void ExportDir::SetNumberOfNames(DWORD number_of_names) const
{
  Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, 
    NumberOfNames), number_of_names);
}

void ExportDir::SetAddressOfFunctions(DWORD address_of_functions) const
{
  Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, 
    AddressOfFunctions), address_of_functions);
}

void ExportDir::SetAddressOfNames(DWORD address_of_names) const
{
  Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, 
    AddressOfNames), address_of_names);
}

void ExportDir::SetAddressOfNameOrdinals(DWORD address_of_name_ordinals) const
{
  Write(*impl_->process_, impl_->base_ + FIELD_OFFSET(IMAGE_EXPORT_DIRECTORY, 
    AddressOfNameOrdinals), address_of_name_ordinals);
}

bool operator==(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(ExportDir const& lhs, ExportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, ExportDir const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, ExportDir const& rhs)
{
  return (lhs << rhs.GetBase());
}

}
