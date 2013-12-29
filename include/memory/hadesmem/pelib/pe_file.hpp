// Copyright (C) 2010-2013 Joshua Boyce.
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
#include <hadesmem/read.hpp>

// TODO: Extra sanity checking in all components. E.g. Check
// NumberOfRvaAndSizes in NtHeaders before attempting to retrieve a data dir.

// TODO: Helper functions such as FindExport, FindImport, HasDataDir,
// GetArchitecture, IsDotNet, GetPDB, etc.

// TODO: Move PeLib code into PeLib namespace.

// TODO: Rewrite PeLib to allow writes from scratch (building new PE file
// sections, data, etc. or even an entire mew PE file). This includes full
// support for writing back to existing PE files also, including automatically
// performing adjustments where required to fit in new data or remove
// unnecessary space.

// TODO: Decouple PeFile from Process. It should use a generic interface
// instead.	Another potential alternative would be policy classes. Needs
// investigation.

// TODO: Decouple PeLib from the architecture being compiled for. Required
// for cross-architecture support in hadesmem.

// TODO: Support more of the PE file format.
// Overlay data.
// Resource directory.
// Exception directory.
// Relocation directory.
// Security directory.
// Debug directory.
// Load config directory.
// Delay import directory.
// Bound import directory.
// IAT(as opposed to Import) directory.
// CLR runtime directory support.
// DOS stub.
// Rich header.
// Checksum.
// etc.

// TODO: Improve PeLib support for pathological cases like Corkami tests. We
// should not only ensure we don't crash, but we should also ensure we're
// actually getting the right data out!

// TODO: Where possible, document the SHA1 of example files which prompted the
// addition of corner cases.

namespace hadesmem
{

// TODO: Investigate whether there is a better way to implement this.
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
    : base_(static_cast<PBYTE>(address)), type_(type), size_(size)
  {
    HADESMEM_DETAIL_ASSERT(base_ != 0);
    if (type == PeFileType::Data && !size)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Invalid file size."));
    }

    // Process is not actually used but we take it anyway for
    // interface symetry and in case we want to change the
    // implementation to use it later without breaking the interface.
    (void)process;
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
    // TODO: Improve the logic here using the information in "Zero section PE
    // file" in ReversingLabs "Undocumented PECOFF" whitepaper.
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
      return base + rva;
    }

    // Apparently on XP it's possible to load a PE with a SizeOfImage of only
    // 0x2e. Treat anything outside of that as invalid. For an example see
    // foldedhdr.exe from the Corkami PE corpus.
    // TODO: According to ReversingLabs "Undocumented PECOFF" whitepaper,
    // SizeOfImage should be rounded up using SectionAlignment. Investigate
    // this.
    if (rva > nt_headers.OptionalHeader.SizeOfImage)
    {
      return nullptr;
    }

    IMAGE_SECTION_HEADER* ptr_section_header =
      reinterpret_cast<PIMAGE_SECTION_HEADER>(
        ptr_nt_headers + offsetof(IMAGE_NT_HEADERS, OptionalHeader) +
        nt_headers.FileHeader.SizeOfOptionalHeader);
    IMAGE_SECTION_HEADER section_header =
      Read<IMAGE_SECTION_HEADER>(process, ptr_section_header);

    bool in_header = true;
    for (WORD i = 0; i < num_sections; ++i)
    {
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
        rva +=
          section_header.PointerToRawData &
          ~((std::max)(0x200UL, nt_headers.OptionalHeader.FileAlignment) - 1);

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

      section_header =
        Read<IMAGE_SECTION_HEADER>(process, ++ptr_section_header);
    }

    if (in_header && rva < pe_file.GetSize())
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
}
