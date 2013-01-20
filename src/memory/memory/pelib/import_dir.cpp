// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/import_dir.hpp"

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

struct ImportDir::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file, 
    PIMAGE_IMPORT_DESCRIPTOR imp_desc)
    : process_(&process), 
    pe_file_(&pe_file), 
    base_(reinterpret_cast<PBYTE>(imp_desc))
  {
    if (!base_)
    {
      NtHeaders nt_headers(process, pe_file);
      DWORD const import_dir_rva = nt_headers.GetDataDirectoryVirtualAddress(
        PeDataDir::Import);
      DWORD const import_dir_size = nt_headers.GetDataDirectorySize(
        PeDataDir::Import);
      if (!import_dir_rva || !import_dir_size)
      {
        HADESMEM_THROW_EXCEPTION(Error() << 
          ErrorString("Import directory is invalid."));
      }

      base_ = static_cast<PBYTE>(RvaToVa(process, pe_file, import_dir_rva));
    }
  }

  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
};

ImportDir::ImportDir(Process const& process, PeFile const& pe_file, 
  PIMAGE_IMPORT_DESCRIPTOR imp_desc)
  : impl_(new Impl(process, pe_file, imp_desc))
{ }

ImportDir::ImportDir(ImportDir const& other)
  : impl_(new Impl(*other.impl_))
{ }

ImportDir& ImportDir::operator=(ImportDir const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

ImportDir::ImportDir(ImportDir&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ImportDir& ImportDir::operator=(ImportDir&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

ImportDir::~ImportDir()
{ }

PVOID ImportDir::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

DWORD ImportDir::GetOriginalFirstThunk() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, OriginalFirstThunk));
}

DWORD ImportDir::GetTimeDateStamp() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, TimeDateStamp));
}

DWORD ImportDir::GetForwarderChain() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, ForwarderChain));
}

DWORD ImportDir::GetNameRaw() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, Name));
}

std::string ImportDir::GetName() const
{
  return ReadString<char>(*impl_->process_, RvaToVa(*impl_->process_, 
    *impl_->pe_file_, GetNameRaw()));
}

DWORD ImportDir::GetFirstThunk() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, FirstThunk));
}

void ImportDir::SetOriginalFirstThunk(DWORD original_first_thunk) const
{
  return Write(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, OriginalFirstThunk), original_first_thunk);
}

void ImportDir::SetTimeDateStamp(DWORD time_date_stamp) const
{
  return Write(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, TimeDateStamp), time_date_stamp);
}

void ImportDir::SetForwarderChain(DWORD forwarder_chain) const
{
  return Write(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, ForwarderChain), forwarder_chain);
}

void ImportDir::SetNameRaw(DWORD name) const
{
  Write(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, Name), name);
}

void ImportDir::SetName(std::string const& name) const
{
  DWORD name_rva = Read<DWORD>(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, Name));
  if (!name_rva)
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Name RVA is null."));
  }

  PVOID name_ptr = RvaToVa(*impl_->process_, *impl_->pe_file_, name_rva);
  if (!name_ptr)
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Name VA is null."));
  }

  std::string const cur_name = ReadString<char>(*impl_->process_, name_ptr);

  // FIXME: Support allocating space for a new name rather than just 
  // overwriting the existing one.
  if (name.size() > cur_name.size())
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("New name longer than existing name."));
  }

  return WriteString(*impl_->process_, name_ptr, name);
}

void ImportDir::SetFirstThunk(DWORD first_thunk) const
{
  return Write(*impl_->process_, impl_->base_ +  FIELD_OFFSET(
    IMAGE_IMPORT_DESCRIPTOR, FirstThunk), first_thunk);
}

bool operator==(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(ImportDir const& lhs, ImportDir const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, ImportDir const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, ImportDir const& rhs)
{
  return (lhs << rhs.GetBase());
}

}
