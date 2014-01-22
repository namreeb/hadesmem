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

// TODO: Extra sanity checking in all components.

// TODO: Helper functions such as FindExport, FindImport, HasDataDir,
// GetArchitecture, IsDotNet, GetPDB, etc.

// TODO: Move PeLib code into PeLib namespace.

// TODO: Rewrite PeLib to allow writes from scratch (building new PE file
// sections, data, etc. or even an entire mew PE file). This includes full
// support for writing back to existing PE files also, including automatically
// performing adjustments where required to fit in new data or remove
// unnecessary space.

// TODO: Decouple PeFile from Process (and other memory APIs). It should use a
// generic interface instead.	Another potential alternative would be policy
// classes. Needs investigation.

// TODO: Decouple PeLib from the architecture being compiled for. Required
// for cross-architecture support in hadesmem.

// TODO: Support more of the PE file format.
// (Overlay data. Resource directory. Exception directory. Relocation directory.
// Security directory. Debug directory. Load config directory. Delay import
// directory. Bound import directory. IAT(as opposed to Import) directory. CLR
// runtime directory support. DOS stub. Rich header. Checksum. etc.)

// TODO: Where possible, document the SHA1 of example files which prompted the
// addition of corner cases.

// TODO: In some cases we're doing checks in list types, in others we're doing
// it in the underlying type. Make this more consistent. One one hand it makes
// sense to do it in the underlying type for some checks, but on the other hand
// other checks belong more in the list type... Think about things ilke
// ImportDir virtual term or AOI term and which scenarios make more sense. When
// users are using the underlying type directly do we want to detect and throw
// on such tricks, or just let them handle it? Probably best to move all checks
// to the list type, as it's the higher level API. However, whilst AOI trick
// might make sense in the list type, handling the virtual trick makes more
// sense in the underlying type, otherwise basic uses of it could cause a read
// past the end of the buffer... Consider other scenarios other than imports
// also...

// TODO: Reduce dependencies various components have on each other (e.g.
// ImportDir depends on TlsDir for detecting AOI trick, BoundImportDir depends
// on ImportDir to ensure that we have imports before accessing bound imports,
// etc.).

// TODO: We should be far more defensive than we are currently being. Validate
// things such as whether offsets are within the file, within the expected data
// dir, higher than an expected address (e.g. NameOffset for bound imports),
// etc. Even if we can't always bail when we detect an inconsistency, we should
// at least have some trace logging and the option to warn.

namespace hadesmem
{

// TODO: Investigate whether there is a better way to implement this.
// TODO: Rename with 'k' prefix.
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

// TODO: Rename this? It makes it sound like we're converting an RVA to a VA
// based on the ImageBase of the PE file. Perhaps this should be called
// ResolveRva instead?
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
      // Only applies in low alignment, otherwise it's invalid?
      // TODO: Should we be checking SectionAlignment here also, because the
      // Corkami wiki implies that low alignment only applies when
      // SectionAlignment is also lower than its normal lower bound.
      // TODO: Verify this.
      if (nt_headers.OptionalHeader.FileAlignment < 200)
      {
        return base + rva;
      }
      // Also only applies if the RVA is smaller than file alignment?
      // TODO: Verify this also.
      // TODO: It seems this is broken for
      // 00030855940c3b6c50789d203a9ba01d8d4cc0dc (seemingly invalid bound
      // import dir RVA being resolved usign this branch). Worked around
      // temporarily by checking for 0 on OffsetModuleName, perhaps that is the
      // correct fix anyway?
      else if (rva < nt_headers.OptionalHeader.FileAlignment)
      {
        return base + rva;
      }
      else
      {
        return nullptr;
      }
    }

    // Apparently on XP it's possible to load a PE with a SizeOfImage of only
    // 0x2e. Treat anything outside of that as invalid. For an example see
    // tinyXP.exe from the Corkami PE corpus.
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
    void const* const file_end =
      static_cast<std::uint8_t*>(pe_file.GetBase()) + pe_file.GetSize();
    // Virtual section table.
    // TODO: Fix this to handle overlap.
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
      // TODO: Handle overlap.
      // TODO: Verify whether this is correct. Should we perhaps be doing more
      // than just returning nullptr here?
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
      // TODO: Apparently the above isn't always true, and there are extra
      // restrictions (however I haven't yet figured them out exactly, but an
      // experimental fix is in place below). Need to investigate this and fix
      // it properly.
      if (virtual_beg <= rva)
      {
        in_header = false;
      }

      ++ptr_section_header;
    }

    // Doing the same thing as in the SizeOfHeaders check above because we're
    // not sure of better criteria to base it off. Perhaps it's correct now?
    // TODO: Investigate this further and fix it properly.
    if (in_header && rva < pe_file.GetSize())
    {
      // Only applies in low alignment, otherwise it's invalid?
      // TODO: Should we be checking SectionAlignment here also, because the
      // Corkami wiki implies that low alignment only applies when
      // SectionAlignment is also lower than its normal lower bound.
      // TODO: Verify this.
      if (nt_headers.OptionalHeader.FileAlignment < 200)
      {
        return base + rva;
      }
      // Also only applies if the RVA is smaller than file alignment?
      // TODO: Verify this also.
      // TODO: Find a sample that relies on this...
      else if (rva < nt_headers.OptionalHeader.FileAlignment)
      {
        return base + rva;
      }
      else
      {
        // TODO: Verify that this is correct.
        return nullptr;
      }
    }

    // TODO: Need to verify whether this is correct for all cases (and doesn't
    // break other samples). It currently feels like it's a hack and we're
    // missing/overlooking the 'actual' fix.
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

// TODO: Handle virtual termination (need a sample).
// TODO: Move this somewhere more appropriate.
// TODO: Support notifying the caller when the string is EOF terminated
// (out-param?).
template <typename CharT>
std::basic_string<CharT> CheckedReadString(Process const& process,
                                           PeFile const& pe_file,
                                           void* address)
{
  if (pe_file.GetType() == PeFileType::Image)
  {
    // TODO: Is extra bounds checking needed here for EOF?
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
    // TODO: Warn for EOF termination.
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
