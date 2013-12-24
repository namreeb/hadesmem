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

// TODO: Improve PeLib support for pathological cases like Corkami tests.

// TODO: Remove assumptions made about the format where possible. The spec is
// often wrong (or simply ignored), so we need to be permissive (but also
// defensive) if we want to be useful when dealing with hostile files.

// TODO: Full support for writing back to PE file, including automatically
// performing adjustments where required to fit in new data or remove
// unnecessary space.

// TODO: Helper functions such as FindExport, FindImport, HasDataDir,
// GetArchitecture, IsDotNet, GetPDB, etc.

// TODO: Don't crash on data which is malformed, but ignored by the PE loader
// (e.g. ISSetup.dll).

// TODO: Move PeLib code into PeLib namespace.

// TODO: Write/Set methods for PeLib should be non-const.

// TODO: Cache base pointers etc rather than retrieving it manually in every
// getter/setter.

// TODO: Rewrite PeLib to do more ‘caching’ and only read/write on demand.
// Also take into account rewriting to allow writes from scratch (see other
// item) as the getters/setters obviously can’t ever reference memory,
// because there might not be a memory region to reference!

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
// etc.

// TODO: Improve support for loading 'Data' files to handle pathalogical
// cases such as structures overlapping with implicitly zeroed virtual memory
// that only exists in-memory. This will probably require 'manually mapping'
// the data file. But what about files that overwrite data or overlap in
// memory but not on disk? That would potentially cause information loss...
// Keep two copies? How do we know which one to use depending on the scenario
// in that case? Needs more thought and investigation... Another concern
// would be write-back for these pathalogical files... That needs more
// thought also.

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
    // TODO: Check whether FileAlignment and/or SectionAlignment
    // should be checked here. In the specially crafted image I'm
    // testing this against the value is '1' for both anyway, but I'd
    // like to ensure it's not possible for it to be higher, and if
    // it is, whether it would affect the RVA resolution here.
    WORD num_sections = nt_headers.FileHeader.NumberOfSections;
    if (!num_sections)
    {
      // In cases where the PE file has no sections it can apparently also have
      // all sorts of messed up RVAs for data dirs etc... Make sure that none of
      // them lie outside the file, because otherwise simply returning a direct
      // offset from the base wouldn't work anyway...
      // TODO: Validate this.
      if (rva > pe_file.GetSize())
      {
        return nullptr;
      }
      else
      {
        return base + rva;
      }
    }

    // SizeOfHeaders can be arbitrarily large (including the size of the
    // entire file). In this case we simply treat all RVAs as an offset from
    // zero, rather than finding its 'true' location in a section.
    // TODO: Check whether this is correct for all cases.
    if (rva < nt_headers.OptionalHeader.SizeOfHeaders)
    {
      return base + rva;
    }

    // Apparently on XP it's possible to load a PE with a SizeOfImage of only
    // 0x2e. Simply treat anything outside of that as invalid for now, though
    // this is not correct for all cases. For an example see "foldedhdr.exe"
    // from the Corkami PE corpus. It is a PE where the NT headers are partially
    // overwritten by section space, as if the sections were folded back on the
    // header.
    // TODO: Fix this for the case outlined above (and others if applicable).
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
        // invalid.
        // TODO: Fix this for "legitimate" cases (a struct overlapping the
        // virtual area to get 'free' zero fill, etc.).
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
        // TODO: Verify this is correct in all cases...
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
      // TODO: Check whether this is correct for all cases. Should we perhaps
      // instead check whether any sections have a PointerToRawData that is zero
      // (or rounded down to zero)?
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
