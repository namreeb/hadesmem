// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/region_list.hpp>
#include <hadesmem/read.hpp>

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
  explicit PeFile(Process const& process,
                  PVOID address,
                  PeFileType type,
                  DWORD size)
    : process_(&process),
      base_(static_cast<PBYTE>(address)),
      type_(type),
      size_(size)
  {
    HADESMEM_DETAIL_ASSERT(base_ != 0);
    if (type == PeFileType::Data && !size)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Invalid file size."));
    }

    if (type == PeFileType::Image && !size)
    {
      RegionList regions(*process_);
      for (auto const& region : regions)
      {
        if (region.GetAllocBase() == base_)
        {
          SIZE_T const region_size = region.GetSize();
          HADESMEM_DETAIL_ASSERT(region_size <
            (std::numeric_limits<DWORD>::max)());
          size_ += static_cast<DWORD>(region_size);
          HADESMEM_DETAIL_ASSERT(size_ >= region_size);
        }

        if (region.GetAllocBase() > base_)
        {
          break;
        }
      }
    }
  }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  PeFileType GetType() const HADESMEM_DETAIL_NOEXCEPT
  {
    return type_;
  }

  DWORD GetSize() const HADESMEM_DETAIL_NOEXCEPT
  {
    return size_;
  }

private:
  Process const* process_;
  PBYTE base_;
  PeFileType type_;
  DWORD size_;
};

inline bool operator==(PeFile const& lhs, PeFile const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(PeFile const& lhs, PeFile const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(PeFile const& lhs, PeFile const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(PeFile const& lhs, PeFile const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(PeFile const& lhs, PeFile const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(PeFile const& lhs, PeFile const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, PeFile const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, PeFile const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
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
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Invalid DOS header."));
    }

    BYTE* ptr_nt_headers = base + dos_header.e_lfanew;
    IMAGE_NT_HEADERS nt_headers =
      Read<IMAGE_NT_HEADERS>(process, ptr_nt_headers);
    if (nt_headers.Signature != IMAGE_NT_SIGNATURE)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Invalid NT headers."));
    }

    // Windows will load specially crafted images with no sections.
    WORD num_sections = nt_headers.FileHeader.NumberOfSections;
    if (!num_sections)
    {
      // In cases where the PE file has no sections it can apparently also have
      // all sorts of messed up RVAs for data dirs etc... Make sure that none of
      // them lie outside the file, because otherwise simply returning a direct
      // offset from the base wouldn't work anyway...
      if (rva > pe_file.GetSize())
      {
        return nullptr;
      }
      else
      {
        return base + rva;
      }
    }

    // SizeOfHeaders can be arbitrarily large, including the size of the
    // entire file. RVAs inside the headers are treated as an offset from
    // zero, rather than finding the 'true' location in a section.
    if (rva < nt_headers.OptionalHeader.SizeOfHeaders)
    {
      // Only applies in low alignment, otherwise it's invalid?
      if (nt_headers.OptionalHeader.FileAlignment < 200)
      {
        return base + rva;
      }
      // Also only applies if the RVA is smaller than file alignment?
      else if (rva < nt_headers.OptionalHeader.FileAlignment)
      {
        return base + rva;
      }
      else
      {
        return nullptr;
      }
    }

    if (rva > nt_headers.OptionalHeader.SizeOfImage)
    {
      return nullptr;
    }

    IMAGE_SECTION_HEADER* ptr_section_header =
      reinterpret_cast<PIMAGE_SECTION_HEADER>(
        ptr_nt_headers + offsetof(IMAGE_NT_HEADERS, OptionalHeader) +
        nt_headers.FileHeader.SizeOfOptionalHeader);
    void const* const file_end =
      static_cast<std::uint8_t*>(pe_file.GetBase()) + pe_file.GetSize();
    // Virtual section table.
    if (ptr_section_header >= file_end)
    {
      if (rva > pe_file.GetSize())
      {
        return nullptr;
      }
      else
      {
        return base + rva;
      }
    }

    bool in_header = true;
    for (WORD i = 0; i < num_sections; ++i)
    {
      // For a virtual section header, simply return nullptr. (Similar to above,
      // except this time only the Nth entry onwards is virtual, rather than all
      // the headers.)
      if (ptr_section_header + 1 > file_end)
      {
        return nullptr;
      }

      auto const section_header =
        Read<IMAGE_SECTION_HEADER>(process, ptr_section_header);

      DWORD const virtual_beg = section_header.VirtualAddress;
      DWORD const virtual_size = section_header.Misc.VirtualSize;
      DWORD const raw_size = section_header.SizeOfRawData;
      // If VirtualSize is zero then SizeOfRawData is used.
      DWORD const virtual_end =
        virtual_beg + (virtual_size ? virtual_size : raw_size);
      if (virtual_beg <= rva && rva < virtual_end)
      {
        rva -= virtual_beg;

        // If the RVA is outside the raw data (which would put it in the
        // zero-fill of the virtual data) just return nullptr because it's
        // invalid. Technically files like this will work when loaded by the
        // PE loader due to the sections being mapped differention in memory
        // to on disk, but if you want to inspect the file in that manner you
        // should just use LoadLibrary with the appropriate flags for your
        // scenario and then use PeFileType::Image.
        if (rva > raw_size)
        {
          return nullptr;
        }

        // If PointerToRawData is less than 0x200 it is rounded
        // down to 0. Safe to mask it off unconditionally because
        // it must be a multiple of FileAlignment.
        rva += section_header.PointerToRawData & ~(0x1FFUL);

        // If the RVA now lies outside the actual file just return nullptr
        // because it's invalid.
        if (rva >= pe_file.GetSize())
        {
          return nullptr;
        }

        return base + rva;
      }

      // This should be the 'normal' case. However sometimes the RVA is at a
      // lower address than any of the sections, so we want to detect this so we
      // can just treat the RVA as an offset from the module base (similar to
      // when the image is loaded).
      if (virtual_beg <= rva)
      {
        in_header = false;
      }

      ++ptr_section_header;
    }

    // Doing the same thing as in the SizeOfHeaders check above because we're
    // not sure of better criteria to base it off. Perhaps it's correct now?
    if (in_header && rva < pe_file.GetSize())
    {
      // Only applies in low alignment, otherwise it's invalid?
      if (nt_headers.OptionalHeader.FileAlignment < 200)
      {
        return base + rva;
      }
      // Also only applies if the RVA is smaller than file alignment?
      else if (rva < nt_headers.OptionalHeader.FileAlignment)
      {
        return base + rva;
      }
      else
      {
        return nullptr;
      }
    }

    // Sample: nullSOH-XP (Corkami PE Corpus)
    if (rva < nt_headers.OptionalHeader.SizeOfImage && rva < pe_file.GetSize())
    {
      return base + rva;
    }

    return nullptr;
  }
  else if (type == PeFileType::Image)
  {
    return rva ? (base + rva) : nullptr;
  }
  else
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("Unhandled file type."));
  }
}

namespace detail
{

template <typename CharT>
std::basic_string<CharT> CheckedReadString(Process const& process,
                                           PeFile const& pe_file,
                                           void* address)
{
  if (pe_file.GetType() == PeFileType::Image)
  {
    return ReadString<CharT>(process, address);
  }
  else if (pe_file.GetType() == PeFileType::Data)
  {
    void* const file_end =
      static_cast<std::uint8_t*>(pe_file.GetBase()) + pe_file.GetSize();
    if (address >= file_end)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error() << ErrorString("Invalid VA."));
    }
    // Handle EOF termination.
    // Sample: maxsecXP.exe (Corkami PE Corpus)
    return ReadStringBounded<CharT>(process, address, file_end);
  }
  else
  {
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("Unknown PE file type."));
  }
}
}
}
