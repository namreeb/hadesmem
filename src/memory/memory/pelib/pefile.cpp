// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/pelib/pefile.hpp"

#include <cstddef>
#include <iostream>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/read.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

namespace pelib
{

namespace
{

PVOID RvaToVaForImage(Process const& /*process*/, PeFile const& pe_file, 
  DWORD rva)
{
  // TODO: Ensure it's correct to return nullptr for a 0 rva (rather than the 
  // base address of the PE file).
  return rva ? (static_cast<PBYTE>(pe_file.GetBase()) + rva) : nullptr;
}

PVOID RvaToVaForData(Process const& process, PeFile const& pe_file, DWORD rva)
{
  // TODO: Ensure it's correct to return nullptr for a 0 rva (rather than the 
  // base address of the PE file).
  if (!rva)
  {
    return nullptr;
  }

  PVOID const base = pe_file.GetBase();

  auto const dos_header = Read<IMAGE_DOS_HEADER>(process, base);
  if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Invalid DOS header."));
  }

  PBYTE const ptr_nt_headers = static_cast<PBYTE>(base) + dos_header.e_lfanew;
  auto const nt_headers = Read<IMAGE_NT_HEADERS>(process, ptr_nt_headers);
  if (nt_headers.Signature != IMAGE_NT_SIGNATURE)
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Invalid NT headers."));
  }

  auto const ptr_section_hdr = ptr_nt_headers + 
    offsetof(IMAGE_NT_HEADERS, OptionalHeader) + 
    nt_headers.FileHeader.SizeOfOptionalHeader;
  auto const section_hdrs = ReadVector<IMAGE_SECTION_HEADER>(process, 
    ptr_section_hdr, nt_headers.FileHeader.NumberOfSections);
  for (auto const& section_hdr : section_hdrs)
  {
    if (section_hdr.VirtualAddress <= rva && 
      (section_hdr.VirtualAddress + section_hdr.Misc.VirtualSize) > rva)
    {
      rva -= section_hdr.VirtualAddress;
      rva += section_hdr.PointerToRawData;
      return static_cast<PBYTE>(base) + rva;
    }
  }

  return nullptr;
}

}

PeFile::PeFile(Process const* process, PVOID base, PeFileType type)
  : process_(process), 
  base_(base), 
  type_(type)
{
  BOOST_ASSERT(process != nullptr);
  BOOST_ASSERT(base != 0);
}

PeFile::PeFile(PeFile const& other) BOOST_NOEXCEPT
  : process_(other.process_), 
  base_(other.base_), 
  type_(other.type_)
{ }

PeFile& PeFile::operator=(PeFile const& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  base_ = other.base_;
  type_ = other.type_;

  return *this;
}

PeFile::PeFile(PeFile&& other) BOOST_NOEXCEPT
  : process_(other.process_), 
  base_(other.base_), 
  type_(other.type_)
{
  other.process_ = nullptr;
  other.base_ = nullptr;
}

PeFile& PeFile::operator=(PeFile&& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  other.process_ = nullptr;
  base_ = other.base_;
  other.base_ = nullptr;
  type_ = other.type_;
  
  return *this;
}

PeFile::~PeFile()
{ }

PVOID PeFile::GetBase() const BOOST_NOEXCEPT
{
  return base_;
}

PeFileType PeFile::GetType() const BOOST_NOEXCEPT
{
  return type_;
}

bool operator==(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT
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

PVOID RvaToVa(Process const& process, PeFile const& pe_file, DWORD rva)
{
  switch (pe_file.GetType())
  {
  case PeFileType::Image:
    return RvaToVaForImage(process, pe_file, rva);
  case PeFileType::Data:
    return RvaToVaForData(process, pe_file, rva);
  }

  BOOST_ASSERT(false);
  return nullptr;
}

}

}
