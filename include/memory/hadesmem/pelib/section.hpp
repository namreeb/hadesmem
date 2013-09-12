// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <iosfwd>
#include <memory>
#include <string>
#include <cstddef>
#include <ostream>
#include <utility>

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

class Section
{
public:
  explicit Section(Process const& process, PeFile const& pe_file, WORD number)
    : process_(&process), 
    pe_file_(&pe_file), 
    number_(number), 
    base_(nullptr)
  {
    NtHeaders const nt_headers(process, pe_file);
    if (number >= nt_headers.GetNumberOfSections())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
        ErrorString("Invalid section number."));
    }
    
    PIMAGE_SECTION_HEADER section_header = 
      reinterpret_cast<PIMAGE_SECTION_HEADER>(static_cast<PBYTE>(
      nt_headers.GetBase()) + offsetof(IMAGE_NT_HEADERS, 
      OptionalHeader) + nt_headers.GetSizeOfOptionalHeader()) + number;
    
    base_ = reinterpret_cast<PBYTE>(section_header);
  }

  Section(Section const& other) HADESMEM_DETAIL_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    number_(other.number_), 
    base_(other.base_)
  { }
  
  Section& operator=(Section const& other) HADESMEM_DETAIL_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    number_ = other.number_;
    base_ = other.base_;

    return *this;
  }

  Section(Section&& other) HADESMEM_DETAIL_NOEXCEPT
    : process_(other.process_), 
    pe_file_(other.pe_file_), 
    number_(other.number_), 
    base_(other.base_)
  { }
  
  Section& operator=(Section&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    process_ = other.process_;
    pe_file_ = other.pe_file_;
    number_ = other.number_;
    base_ = other.base_;

    return *this;
  }
  
  ~Section() HADESMEM_DETAIL_NOEXCEPT
  { }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  WORD GetNumber() const HADESMEM_DETAIL_NOEXCEPT
  {
    return number_;
  }
  
  std::string GetName() const
  {
    std::array<char, 8> const name_raw = Read<char, 8>(*process_, base_ + 
      offsetof(IMAGE_SECTION_HEADER, Name));

    std::string name;
    for (std::size_t i = 0; i < 8 && name_raw[i]; ++i)
    {
      name += name_raw[i];
    }

    return name;
  }

  DWORD GetVirtualAddress() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      VirtualAddress));
  }

  DWORD GetVirtualSize() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      Misc.VirtualSize));
  }

  DWORD GetSizeOfRawData() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      SizeOfRawData));
  }

  DWORD GetPointerToRawData() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      PointerToRawData));
  }

  DWORD GetPointerToRelocations() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      PointerToRelocations));
  }

  DWORD GetPointerToLinenumbers() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      PointerToLinenumbers));
  }

  WORD GetNumberOfRelocations() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      NumberOfRelocations));
  }

  WORD GetNumberOfLinenumbers() const
  {
    return Read<WORD>(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      NumberOfLinenumbers));
  }

  DWORD GetCharacteristics() const
  {
    return Read<DWORD>(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      Characteristics));
  }

  void SetName(std::string const& Name)
  {
    WriteString(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      Name), Name);
  }

  void SetVirtualAddress(DWORD VirtualAddress)
  {
    Write(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      VirtualAddress), VirtualAddress);
  }

  void SetVirtualSize(DWORD VirtualSize)
  {
    Write(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      Misc.VirtualSize), VirtualSize);
  }

  void SetSizeOfRawData(DWORD SizeOfRawData)
  {
    Write(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      SizeOfRawData), SizeOfRawData);
  }

  void SetPointerToRawData(DWORD PointerToRawData)
  {
    Write(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      PointerToRawData), PointerToRawData);
  }

  void SetPointerToRelocations(DWORD PointerToRelocations)
  {
    Write(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      PointerToRelocations), PointerToRelocations);
  }

  void SetPointerToLinenumbers(DWORD PointerToLinenumbers)
  {
    Write(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      PointerToLinenumbers), PointerToLinenumbers);
  }

  void SetNumberOfRelocations(WORD NumberOfRelocations)
  {
    Write(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      NumberOfRelocations), NumberOfRelocations);
  }

  void SetNumberOfLinenumbers(WORD NumberOfLinenumbers)
  {
    Write(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      NumberOfLinenumbers), NumberOfLinenumbers);
  }

  void SetCharacteristics(DWORD Characteristics)
  {
    Write(*process_, base_ + offsetof(IMAGE_SECTION_HEADER, 
      Characteristics), Characteristics);
  }

private:
  template <typename SectionT>
  friend class SectionIterator;

  explicit Section(Process const& process, PeFile const& pe_file, WORD number, 
    PVOID base)
    : process_(&process), 
    pe_file_(&pe_file), 
    number_(number), 
    base_(static_cast<PBYTE>(base))
  { }
  
  Process const* process_;
  PeFile const* pe_file_;
  WORD number_;
  PBYTE base_;
};

inline bool operator==(Section const& lhs, Section const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(Section const& lhs, Section const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Section const& lhs, Section const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(Section const& lhs, Section const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(Section const& lhs, Section const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(Section const& lhs, Section const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, Section const& rhs)
{
  std::locale old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Section const& rhs)
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
