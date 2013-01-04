// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/section.hpp"

#include <array>
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

struct Section::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file, WORD number)
    : process_(&process), 
    pe_file_(&pe_file), 
    number_(number), 
    base_(static_cast<PBYTE>(pe_file.GetBase()))
  {
    NtHeaders const nt_headers(process, pe_file);
    if (number >= nt_headers.GetNumberOfSections())
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Invalid section number."));
    }
    
    PIMAGE_SECTION_HEADER section_header = 
      reinterpret_cast<PIMAGE_SECTION_HEADER>(static_cast<PBYTE>(
      nt_headers.GetBase()) + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader) + nt_headers.GetSizeOfOptionalHeader()) + number;
    
    base_ = reinterpret_cast<PBYTE>(section_header);
  }

  Process const* process_;
  PeFile const* pe_file_;
  WORD number_;
  PBYTE base_;
};

Section::Section(Process const& process, PeFile const& pe_file, WORD number)
  : impl_(new Impl(process, pe_file, number))
{ }

Section::Section(Section const& other)
  : impl_(new Impl(*other.impl_))
{ }

Section& Section::operator=(Section const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

Section::Section(Section&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

Section& Section::operator=(Section&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

Section::~Section()
{ }

PVOID Section::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

WORD Section::GetNumber() const
{
  return impl_->number_;
}

std::string Section::GetName() const
{
  std::array<char, 8> const name_raw = Read<char, 8>(*impl_->process_, 
    impl_->base_ + offsetof(IMAGE_SECTION_HEADER, Name));

  std::string name;
  for (std::size_t i = 0; i < 8 && name_raw[i]; ++i)
  {
    name += name_raw[i];
  }

  return name;
}

DWORD Section::GetVirtualAddress() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_SECTION_HEADER, VirtualAddress));
}

DWORD Section::GetVirtualSize() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_SECTION_HEADER, Misc.VirtualSize));
}

DWORD Section::GetSizeOfRawData() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_SECTION_HEADER, SizeOfRawData));
}

DWORD Section::GetPointerToRawData() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_SECTION_HEADER, PointerToRawData));
}

DWORD Section::GetPointerToRelocations() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_SECTION_HEADER, PointerToRelocations));
}

DWORD Section::GetPointerToLinenumbers() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_SECTION_HEADER, PointerToLinenumbers));
}

WORD Section::GetNumberOfRelocations() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_SECTION_HEADER, NumberOfRelocations));
}

WORD Section::GetNumberOfLinenumbers() const
{
  return Read<WORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_SECTION_HEADER, NumberOfLinenumbers));
}

DWORD Section::GetCharacteristics() const
{
  return Read<DWORD>(*impl_->process_, impl_->base_ + offsetof(
    IMAGE_SECTION_HEADER, Characteristics));
}

void Section::SetName(std::string const& Name) const
{
  WriteString(*impl_->process_, impl_->base_ + offsetof(IMAGE_SECTION_HEADER, 
    Name), Name);
}

void Section::SetVirtualAddress(DWORD VirtualAddress) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_SECTION_HEADER, 
    VirtualAddress), VirtualAddress);
}

void Section::SetVirtualSize(DWORD VirtualSize) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_SECTION_HEADER, 
    Misc.VirtualSize), VirtualSize);
}

void Section::SetSizeOfRawData(DWORD SizeOfRawData) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_SECTION_HEADER, 
    SizeOfRawData), SizeOfRawData);
}

void Section::SetPointerToRawData(DWORD PointerToRawData) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_SECTION_HEADER, 
    PointerToRawData), PointerToRawData);
}

void Section::SetPointerToRelocations(DWORD PointerToRelocations) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_SECTION_HEADER, 
    PointerToRelocations), PointerToRelocations);
}

void Section::SetPointerToLinenumbers(DWORD PointerToLinenumbers) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_SECTION_HEADER, 
    PointerToLinenumbers), PointerToLinenumbers);
}

void Section::SetNumberOfRelocations(WORD NumberOfRelocations) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_SECTION_HEADER, 
    NumberOfRelocations), NumberOfRelocations);
}

void Section::SetNumberOfLinenumbers(WORD NumberOfLinenumbers) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_SECTION_HEADER, 
    NumberOfLinenumbers), NumberOfLinenumbers);
}

void Section::SetCharacteristics(DWORD Characteristics) const
{
  Write(*impl_->process_, impl_->base_ + offsetof(IMAGE_SECTION_HEADER, 
    Characteristics), Characteristics);
}

bool operator==(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(Section const& lhs, Section const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, Section const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, Section const& rhs)
{
  return (lhs << rhs.GetBase());
}

}
