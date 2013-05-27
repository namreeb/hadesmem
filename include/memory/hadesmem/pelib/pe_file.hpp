// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <iosfwd>
#include <memory>
#include <cstddef>
#include <ostream>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/read.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/assert.hpp>

namespace hadesmem
{

enum class PeFileType
{
  Image, 
  Data
};

class PeFile
{
public:
  explicit PeFile(Process const& process, PVOID address, PeFileType type)
    : process_(&process), 
    base_(static_cast<PBYTE>(address)), 
    type_(type)
  {
    HADESMEM_ASSERT(base_ != 0);
  }

  PeFile(PeFile const& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    base_(other.base_), 
    type_(other.type_)
  { }
  
  PeFile& operator=(PeFile const& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    base_ = other.base_;
    type_ = other.type_;

    return *this;
  }

  PeFile(PeFile&& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    base_(other.base_), 
    type_(other.type_)
  { }
  
  PeFile& operator=(PeFile&& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    base_ = other.base_;
    type_ = other.type_;

    return *this;
  }
  
  ~PeFile() HADESMEM_NOEXCEPT
  { }

  PVOID GetBase() const HADESMEM_NOEXCEPT
  {
    return base_;
  }

  PeFileType GetType() const HADESMEM_NOEXCEPT
  {
    return type_;
  }

private:
    Process const* process_;
    PBYTE base_;
    PeFileType type_;
};

inline bool operator==(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, PeFile const& rhs)
{
  return (lhs << rhs.GetBase());
}

inline std::wostream& operator<<(std::wostream& lhs, PeFile const& rhs)
{
  return (lhs << rhs.GetBase());
}

inline PVOID RvaToVa(Process const& process, PeFile const& pe_file, DWORD rva)
{
  PeFileType const type = pe_file.GetType();
  PBYTE base = static_cast<PBYTE>(pe_file.GetBase());

  if (type == PeFileType::Data)
  {
    if (!rva)
    {
      return nullptr;
    }

    IMAGE_DOS_HEADER dos_header = Read<IMAGE_DOS_HEADER>(process, base);
    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("Invalid DOS header."));
    }

    BYTE* ptr_nt_headers = base + dos_header.e_lfanew;
    IMAGE_NT_HEADERS nt_headers = Read<IMAGE_NT_HEADERS>(process, 
      ptr_nt_headers);
    if (nt_headers.Signature != IMAGE_NT_SIGNATURE)
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("Invalid NT headers."));
    }

    IMAGE_SECTION_HEADER* ptr_section_header = 
      reinterpret_cast<PIMAGE_SECTION_HEADER>(ptr_nt_headers + offsetof(
      IMAGE_NT_HEADERS, OptionalHeader) + nt_headers.FileHeader.
      SizeOfOptionalHeader);
    IMAGE_SECTION_HEADER section_header = Read<IMAGE_SECTION_HEADER>(
      process, ptr_section_header);
    
    WORD num_sections = nt_headers.FileHeader.NumberOfSections;

    // Windows will load specially crafted images with no sections.
    // TODO: Check whether FileAlignment and/or SectionAlignment should be 
    // checked here. In the specially crafted image I'm testing this against 
    // the value is '1' for both anyway, but I'd like to ensure it's not 
    // possible for it to be higher, and if it is, whether it would affect 
    // the RVA resolution here.
    if (!num_sections)
    {
      return base + rva;
    }

    for (WORD i = 0; i < num_sections; ++i)
    {
      DWORD const virtual_beg = section_header.VirtualAddress;
      DWORD const virtual_end = virtual_beg + section_header.Misc.VirtualSize;
      if (virtual_beg <= rva && rva < virtual_end)
      {
        rva -= virtual_beg;
        // If PointerToRawData is less than 0x200 it is rounded down to 0.
        // TODO: Check if this is the correct way to be doing this. In the 
        // Windows loader this is probably done as part of a more complex 
        // mask, containing logic that's missing here.
        if (section_header.PointerToRawData >= 0x200)
        {
          rva += section_header.PointerToRawData;
        }

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
