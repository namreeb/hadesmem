// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

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
      base_(nullptr),
      data_()
  {
    NtHeaders const nt_headers(process, pe_file);
    if (number >= nt_headers.GetNumberOfSections())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Invalid section number."));
    }

    PIMAGE_SECTION_HEADER section_header =
      reinterpret_cast<PIMAGE_SECTION_HEADER>(
        static_cast<PBYTE>(nt_headers.GetBase()) +
        offsetof(IMAGE_NT_HEADERS, OptionalHeader) +
        nt_headers.GetSizeOfOptionalHeader()) +
      number;

    base_ = reinterpret_cast<PBYTE>(section_header);

    UpdateRead();
  }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  void UpdateRead()
  {
    data_ = Read<IMAGE_SECTION_HEADER>(*process_, base_);
  }

  void UpdateWrite()
  {
    Write(*process_, base_, data_);
  }

  WORD GetNumber() const HADESMEM_DETAIL_NOEXCEPT
  {
    return number_;
  }

  std::string GetName() const
  {
    std::string name;
    for (std::size_t i = 0; i < 8 && data_.Name[i]; ++i)
    {
      name += data_.Name[i];
    }

    return name;
  }

  DWORD GetVirtualAddress() const
  {
    return data_.VirtualAddress;
  }

  DWORD GetVirtualSize() const
  {
    return data_.Misc.VirtualSize;
  }

  DWORD GetSizeOfRawData() const
  {
    return data_.SizeOfRawData;
  }

  DWORD GetPointerToRawData() const
  {
    return data_.PointerToRawData;
  }

  DWORD GetPointerToRelocations() const
  {
    return data_.PointerToRelocations;
  }

  DWORD GetPointerToLinenumbers() const
  {
    return data_.PointerToLinenumbers;
  }

  WORD GetNumberOfRelocations() const
  {
    return data_.NumberOfRelocations;
  }

  WORD GetNumberOfLinenumbers() const
  {
    return data_.NumberOfLinenumbers;
  }

  DWORD GetCharacteristics() const
  {
    return data_.Characteristics;
  }

  void SetName(std::string const& name)
  {
    if (name.size() > 8)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("New section name too large."));
    }

    std::copy(std::begin(name),
              std::end(name),
              reinterpret_cast<char*>(&data_.Name[0]));
    if (name.size() != 8)
    {
      data_.Name[name.size()] = '\0';
    }
  }

  void SetVirtualAddress(DWORD virtual_address)
  {
    data_.VirtualAddress = virtual_address;
  }

  void SetVirtualSize(DWORD virtual_size)
  {
    data_.Misc.VirtualSize = virtual_size;
  }

  void SetSizeOfRawData(DWORD size_of_raw_data)
  {
    data_.SizeOfRawData = size_of_raw_data;
  }

  void SetPointerToRawData(DWORD pointer_to_raw_data)
  {
    data_.PointerToRawData = pointer_to_raw_data;
  }

  void SetPointerToRelocations(DWORD pointer_to_relocations)
  {
    data_.PointerToRelocations = pointer_to_relocations;
  }

  void SetPointerToLinenumbers(DWORD pointer_to_linenumbers)
  {
    data_.PointerToLinenumbers = pointer_to_linenumbers;
  }

  void SetNumberOfRelocations(WORD number_of_relocations)
  {
    data_.NumberOfRelocations = number_of_relocations;
  }

  void SetNumberOfLinenumbers(WORD number_of_linenumbers)
  {
    data_.NumberOfLinenumbers = number_of_linenumbers;
  }

  void SetCharacteristics(DWORD characteristics)
  {
    data_.Characteristics = characteristics;
  }

private:
  template <typename SectionT> friend class SectionIterator;

  explicit Section(Process const& process,
                   PeFile const& pe_file,
                   WORD number,
                   PVOID base)
    : process_(&process),
      pe_file_(&pe_file),
      number_(number),
      base_(static_cast<PBYTE>(base)),
      data_()
  {
    UpdateRead();
  }

  Process const* process_;
  PeFile const* pe_file_;
  WORD number_;
  PBYTE base_;
  IMAGE_SECTION_HEADER data_;
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
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Section const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}
}

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif
