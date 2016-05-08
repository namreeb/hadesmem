// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/region_alloc_size.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/region_list.hpp>
#include <hadesmem/read.hpp>

// TODO: Add proper regression tests for PeLib. This will require running
// against a known sample set with reference data to compare to.

// TODO: Add better checking to all types that we're not reading outside the
// file/buffer. We usually check the RVA/VA, but we don't always validate the
// size. Also need to check for overflow etc. when using size.

// TODO: Add option to manually map data files, so we can apply fixups etc which
// in turn means we can basically just treat it as an Image in memory. It also
// means we can handle weird loader differences with different mapping flags for
// XP vs 7 vs 8 etc.

// TODO: Investigate what the point of IMAGE_DIRECTORY_ENTRY_IAT is. Used by
// virtsectblXP.exe. Does it actually have to be the IAT (i.e. FirstThunk)? I'm
// pretty sure it's different in some cases... Add warning in Dump for this and
// run a full scan.

// TODO: Decouple PeLib from Process so we can operate directly on
// files/memory/etc. Need some sort of abstraction to replace Process (and the
// accompanying calls to Read/Write/etc.). Dependency on hadesmem APIs in
// general should be removed, as ideally we could make the PeFile code
// OS-independent as all we're doing is parsing files.

// TODO: Move to an attribute based system for warning on malformed or
// suspicious files. Also important for testing, so we can ensure certain
// branches are hit.

// TODO: Return correctly typed pointers from GetBase, GetStart, etc. (Adjust
// ostream overloads to cast to void*).

// TODO: Add noexcept to all functions which are now using cached data and can
// no longer throw.

// TODO: Helper functions such as FindExport, FindImport, HasDataDir,
// GetArchitecture, IsDotNet, GetPDB, etc.

// TODO: Move to 'pelib' namespace.

// TODO: Rewrite PeLib to allow writes from scratch (building new PE file
// sections, data, etc. or even an entire mew PE file). This includes full
// support for writing back to existing PE files also, including automatically
// performing adjustments where required to fit in new data or remove
// unnecessary space.

// TODO: Support more of the PE file format. (Overlay data. Resource directory.
// Exception directory. Relocation directory. Security directory. Debug
// directory. Load config directory. Delay import directory. Bound import
// directory. IAT(as opposed to Import) directory. CLR runtime directory
// support. DOS stub. Rich header. Checksum. etc.)

// TODO: Reduce dependencies various components have on each other (e.g.
// ImportDir depends on TlsDir for detecting AOI trick, BoundImportDir depends
// on ImportDir to ensure that we have imports before accessing bound imports,
// etc.).

// TODO: We should be far more defensive than we are currently being. Validate
// things such as whether offsets are within the file, within the expected data
// dir, higher than an expected address (e.g. NameOffset for bound imports),
// etc.

// TODO: Where possible, document the SHA1 of example files which prompted the
// addition of corner cases, and add them to online repository.

// TODO: Proper overflow checking everywhere.

// TODO: In some cases we're doing checks in list types, in others we're doing
// it in the underlying type. Make this more consistent. Also think about how
// this interacts with the proposed move to an attribute based system.

// TODO: Write and use synthetic files similar to Corkami as part of unit tests.
// One sample per trick/feature/etc.

// TODO: Stop hard-swallowing warnings/errors in PeLib types, they should be
// exposed to the tools (e.g. so Dump can warn). This should probably be fixed
// with the aformentioned attributes suggestion.

namespace hadesmem
{
// TODO: Investigate if there is a better way to implement PeLib rather than
// branching on PeFileType everywhere.
enum class PeFileType
{
  kImage,
  kData
};

class PeFile
{
public:
  explicit PeFile(Process const& process,
                  void* address,
                  PeFileType type,
                  DWORD size)
    : process_{&process},
      base_{static_cast<std::uint8_t*>(address)},
      type_{type},
      size_{size}
  {
    HADESMEM_DETAIL_ASSERT(base_ != 0);
    if (type == PeFileType::kData && !size)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid file size."});
    }

    if (type == PeFileType::kImage && !size)
    {
      try
      {
        Module const module{process, reinterpret_cast<HMODULE>(address)};
        size_ = module.GetSize();
      }
      catch (...)
      {
        auto const module_region_size =
          detail::GetModuleRegionSize(*process_, base_);
        HADESMEM_DETAIL_ASSERT(module_region_size <
                               (std::numeric_limits<DWORD>::max)());
        size_ = static_cast<DWORD>(module_region_size);
      }
    }

    // Not erroring out anywhere here in order to retain back-compat.
    // TODO: Do this properly as part of the rewrite.
    try
    {
      if (size_ > sizeof(IMAGE_DOS_HEADER))
      {
        auto const nt_hdrs_ofs =
          Read<IMAGE_DOS_HEADER>(process, address).e_lfanew;
        if (size_ >= nt_hdrs_ofs + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER))
        {
          auto const nt_hdrs = Read<IMAGE_NT_HEADERS>(
            process, static_cast<std::uint8_t*>(address) + nt_hdrs_ofs);
          if (nt_hdrs.Signature == IMAGE_NT_SIGNATURE &&
              nt_hdrs.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
          {
            is_64_ = true;
          }
        }
      }
    }
    catch (...)
    {
    }
  }

  explicit PeFile(Process const&& process,
                  void* address,
                  PeFileType type,
                  DWORD size) = delete;

  PVOID GetBase() const noexcept
  {
    return base_;
  }

  PeFileType GetType() const noexcept
  {
    return type_;
  }

  DWORD GetSize() const noexcept
  {
    return size_;
  }

  bool Is64() const noexcept
  {
    return is_64_;
  }

private:
  Process const* process_;
  PBYTE base_;
  PeFileType type_;
  DWORD size_;
  bool is_64_{false};
};

inline bool operator==(PeFile const& lhs, PeFile const& rhs) noexcept
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(PeFile const& lhs, PeFile const& rhs) noexcept
{
  return !(lhs == rhs);
}

inline bool operator<(PeFile const& lhs, PeFile const& rhs) noexcept
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(PeFile const& lhs, PeFile const& rhs) noexcept
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(PeFile const& lhs, PeFile const& rhs) noexcept
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(PeFile const& lhs, PeFile const& rhs) noexcept
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

// TODO: Add sample files for all the corner cases we're handling, and ensure it
// is correct, so we can add regression tests.
// TODO: Find a better name for this functions? It's slightly confusing...
// TODO: Measure code coverage of this and other critical functions when writing
// tests to ensure full coverage. Then add attributes and regression tests.
// TODO: Consider if there is a better way to handle virtual VAs other than an
// out param. Attributes?
inline PVOID RvaToVa(Process const& process,
                     PeFile const& pe_file,
                     DWORD rva,
                     bool* virtual_va = nullptr)
{
  if (virtual_va)
  {
    *virtual_va = false;
  }

  PeFileType const type = pe_file.GetType();
  PBYTE base = static_cast<PBYTE>(pe_file.GetBase());

  if (type == PeFileType::kData)
  {
    if (!rva)
    {
      return nullptr;
    }

    IMAGE_DOS_HEADER dos_header = Read<IMAGE_DOS_HEADER>(process, base);
    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid DOS header."});
    }

    BYTE* ptr_nt_headers = base + dos_header.e_lfanew;
    if (Read<DWORD>(process, ptr_nt_headers) != IMAGE_NT_SIGNATURE)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid NT headers."});
    }

    auto const file_header =
      Read<IMAGE_FILE_HEADER>(process, ptr_nt_headers + sizeof(DWORD));

    auto const optional_header_32 =
      pe_file.Is64()
        ? IMAGE_OPTIONAL_HEADER32{}
        : Read<IMAGE_OPTIONAL_HEADER32>(process,
                                        ptr_nt_headers + sizeof(DWORD) +
                                          sizeof(IMAGE_FILE_HEADER));
    auto const optional_header_64 =
      pe_file.Is64()
        ? Read<IMAGE_OPTIONAL_HEADER64>(
            process, ptr_nt_headers + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER))
        : IMAGE_OPTIONAL_HEADER64{};

    DWORD const size_of_headers = pe_file.Is64()
                                    ? optional_header_64.SizeOfHeaders
                                    : optional_header_32.SizeOfHeaders;
    DWORD const file_alignment = pe_file.Is64()
                                   ? optional_header_64.FileAlignment
                                   : optional_header_32.FileAlignment;
    DWORD const size_of_image = pe_file.Is64() ? optional_header_64.SizeOfImage
                                               : optional_header_32.SizeOfImage;

    // A PE file can legally have zero sections, in which case the entire file
    // is executable as though it were a single section whose size is equal to
    // the SizeOfHeaders value rounded up to the nearest page.
    // TODO: Confirm that the comment on rounding is correct, then implement it.
    WORD num_sections = file_header.NumberOfSections;
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

    // SizeOfHeaders can be arbitrarily large, including the size of the entire.
    // RVAs inside the headers are treated as an offset from zero, rather than
    // finding the 'true' location in a section.
    if (rva < size_of_headers)
    {
      // TODO: This probably needs some extra checks as some cases are probably
      // invalid, but I don't know what the checks should be. Need to
      // investigate to see what is allowed and what is not.
      if (rva > pe_file.GetSize() || rva > size_of_image)
      {
        return nullptr;
      }
      else
      {
        return base + rva;
      }
    }

    if (rva > size_of_image)
    {
      return nullptr;
    }

    auto ptr_section_header = reinterpret_cast<PIMAGE_SECTION_HEADER>(
      ptr_nt_headers + offsetof(IMAGE_NT_HEADERS, OptionalHeader) +
      file_header.SizeOfOptionalHeader);
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
        // scenario and then use PeFileType::kImage.
        if (rva > raw_size)
        {
          // It's useful to be able to detect this case as a user for things
          // like exports, where typically a failure to resolve an RVA would be
          // an error/suspicious, but not in the case of a data export where it
          // is normal for the RVA to be in the zero fill of a data segment.
          // TODO: Find other places in this function where we need to set this
          // flag.
          // TODO: Also check section characteristics?
          if (rva < virtual_size && virtual_va)
          {
            *virtual_va = true;
          }

          return nullptr;
        }

        // If PointerToRawData is less than 0x200 it is rounded
        // down to 0.
        if (section_header.PointerToRawData >= 0x200)
        {
          // TODO: Check whether we actually need/want to force alignment here.
          rva += section_header.PointerToRawData & ~(file_alignment - 1);
        }

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
      if (file_alignment < 200)
      {
        return base + rva;
      }
      // Also only applies if the RVA is smaller than file alignment?
      else if (rva < file_alignment)
      {
        return base + rva;
      }
      else
      {
        return nullptr;
      }
    }

    // Sample: nullSOH-XP (Corkami PE Corpus)
    if (rva < size_of_image && rva < pe_file.GetSize())
    {
      return base + rva;
    }

    return nullptr;
  }
  else if (type == PeFileType::kImage)
  {
    return rva ? (base + rva) : nullptr;
  }
  else
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"Unhandled file type."});
  }
}

// TODO: 'Harden' this function against malicious/malformed PE files like is
// done for RvaToVa.
inline DWORD FileOffsetToRva(Process const& process,
                             PeFile const& pe_file,
                             DWORD file_offset)
{
  PeFileType const type = pe_file.GetType();
  PBYTE base = static_cast<PBYTE>(pe_file.GetBase());

  if (type == PeFileType::kData)
  {
    IMAGE_DOS_HEADER dos_header = Read<IMAGE_DOS_HEADER>(process, base);
    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid DOS header."});
    }

    BYTE* ptr_nt_headers = base + dos_header.e_lfanew;
    if (Read<DWORD>(process, ptr_nt_headers) != IMAGE_NT_SIGNATURE)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid NT headers."});
    }

    auto const file_header =
      Read<IMAGE_FILE_HEADER>(process, ptr_nt_headers + sizeof(DWORD));

    auto ptr_section_header = reinterpret_cast<PIMAGE_SECTION_HEADER>(
      ptr_nt_headers + offsetof(IMAGE_NT_HEADERS, OptionalHeader) +
      file_header.SizeOfOptionalHeader);

    WORD num_sections = file_header.NumberOfSections;
    for (WORD i = 0; i < num_sections; ++i)
    {
      auto const section_header =
        Read<IMAGE_SECTION_HEADER>(process, ptr_section_header);

      DWORD const raw_beg = section_header.PointerToRawData;
      DWORD const raw_size = section_header.SizeOfRawData;
      DWORD const raw_end = raw_beg + raw_size;
      if (raw_beg <= file_offset && file_offset < raw_end)
      {
        file_offset -= raw_beg;
        file_offset += section_header.VirtualAddress;
        return file_offset;
      }

      ++ptr_section_header;
    }

    return 0;
  }
  else if (type == PeFileType::kImage)
  {
    return file_offset;
  }
  else
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"Unhandled file type."});
  }
}

namespace detail
{
// TODO: Handle virtual termination.
// TODO: Warn in tools when EOF/Virtual/etc. termination is detected.
// TODO: Move this somewhere more appropriate.
template <typename CharT>
std::basic_string<CharT> CheckedReadString(Process const& process,
                                           PeFile const& pe_file,
                                           void* address)
{
  if (pe_file.GetType() == PeFileType::kImage)
  {
    // TODO: Extra bounds checking to ensure we don't read outside the image in
    // the case that we're reading a string at the end of the file which is not
    // null terminated, and we're on a region boundary.
    return ReadString<CharT>(process, address);
  }
  else if (pe_file.GetType() == PeFileType::kData)
  {
    void* const file_end =
      static_cast<std::uint8_t*>(pe_file.GetBase()) + pe_file.GetSize();
    if (address >= file_end)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{} << ErrorString{"Invalid VA."});
    }
    // Handle EOF termination.
    // Sample: maxsecXP.exe (Corkami PE Corpus)
    return ReadStringBounded<CharT>(process, address, file_end);
  }
  else
  {
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"Unknown PE file type."});
  }
}
}
}
