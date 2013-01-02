// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/pefile.hpp"

#include <cstddef>
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
    base_(static_cast<PBYTE>(address)), 
    type_(type)
  {
    BOOST_ASSERT(base_ != 0);
  }

  Process const* process_;
  PBYTE base_;
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

PVOID RvaToVa(Process const& process, PeFile const& pefile, DWORD rva)
{
  PeFileType const type = pefile.GetType();
  PBYTE base = static_cast<PBYTE>(pefile.GetBase());

  if (type == PeFileType::Data)
  {
    if (!rva)
    {
      return nullptr;
    }

    IMAGE_DOS_HEADER const dos_header = Read<IMAGE_DOS_HEADER>(process, base);
    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Invalid DOS header."));
    }

    BYTE* ptr_nt_headers = base + dos_header.e_lfanew;
    IMAGE_NT_HEADERS const nt_headers = Read<IMAGE_NT_HEADERS>(process, 
      ptr_nt_headers);
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
      process, ptr_section_header);

    WORD num_sections = nt_headers.FileHeader.NumberOfSections;

    for (WORD i = 0; i < num_sections; ++i)
    {
      if (section_header.VirtualAddress <= rva && (section_header.
        VirtualAddress + section_header.Misc.VirtualSize) > rva)
      {
        rva -= section_header.VirtualAddress;
        rva += section_header.PointerToRawData;

        return base + rva;
      }

      section_header = Read<IMAGE_SECTION_HEADER>(process, 
        ++ptr_section_header);
    }

    return nullptr;
  }
  else if (type == PeFileType::Image)
  {
    return rva ? (base + rva) : nullptr;
  }
  else
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Unhandled file type."));
  }
}

}
