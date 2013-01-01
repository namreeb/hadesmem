// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/pefile.hpp"

#include <utility>
#include <iostream>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <winnt.h>

#include "hadesmem/read.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

struct PeFile::Impl
{
  explicit Impl(Process const& process, PVOID address, PeFileType type)
    : process_(&process), 
    base_(address), 
    type_(type)
  {
    BOOST_ASSERT(base_ != 0);
  }

  Process const* process_;
  PVOID base_;
  PeFileType type_;
};

PeFile::PeFile(Process const& process, PVOID address, PeFileType type)
  : impl_(new Impl(process, address, type))
{ }

PeFile::PeFile(PeFile const& other)
  : impl_(new Impl(*other.impl_))
{ }

PeFile& PeFile::operator=(PeFile const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

PeFile::PeFile(PeFile&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

PeFile& PeFile::operator=(PeFile&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

PeFile::~PeFile()
{ }

PVOID PeFile::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

PeFileType PeFile::GetType() const HADESMEM_NOEXCEPT
{
  return impl_->type_;
}

PVOID PeFile::RvaToVa(DWORD rva) const
{
  if (impl_->type_ == PeFileType::Data)
  {
    if (!rva)
    {
      return nullptr;
    }

    IMAGE_DOS_HEADER const dos_header = Read<IMAGE_DOS_HEADER>(
      *impl_->process_, impl_->base_);
    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Invalid DOS header."));
    }

    BYTE* ptr_nt_headers = static_cast<BYTE*>(impl_->base_) + 
      dos_header.e_lfanew;
    IMAGE_NT_HEADERS const nt_headers = Read<IMAGE_NT_HEADERS>(
      *impl_->process_, ptr_nt_headers);
    if (nt_headers.Signature != IMAGE_NT_SIGNATURE)
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Invalid NT headers."));
    }

    IMAGE_SECTION_HEADER* ptr_section_header = 
      reinterpret_cast<PIMAGE_SECTION_HEADER>(ptr_nt_headers + offsetof(
      IMAGE_NT_HEADERS, OptionalHeader) + nt_headers.FileHeader.
      SizeOfOptionalHeader);
    IMAGE_SECTION_HEADER section_header = Read<IMAGE_SECTION_HEADER>(
      *impl_->process_, ptr_section_header);

    WORD num_sections = nt_headers.FileHeader.NumberOfSections;

    for (WORD i = 0; i < num_sections; ++i)
    {
      if (section_header.VirtualAddress <= rva && (section_header.
        VirtualAddress + section_header.Misc.VirtualSize) > rva)
      {
        rva -= section_header.VirtualAddress;
        rva += section_header.PointerToRawData;

        return static_cast<BYTE*>(impl_->base_) + rva;
      }

      section_header = Read<IMAGE_SECTION_HEADER>(*impl_->process_, 
        ++ptr_section_header);
    }

    return nullptr;
  }
  else if (impl_->type_ == PeFileType::Image)
  {
    return rva ? (static_cast<BYTE*>(impl_->base_) + rva) : nullptr;
  }
  else
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Unhandled file type."));
  }
}

bool operator==(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, PeFile const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, PeFile const& rhs)
{
  return (lhs << rhs.GetBase());
}

}
