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

// TODO: Add proper regression tests for PeLib.

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
                  void* address,
                  PeFileType type,
                  DWORD size)
    : process_{&process},
      base_{static_cast<std::uint8_t*>(address)},
      type_{type},
      size_{size}
  {
    HADESMEM_DETAIL_ASSERT(base_ != 0);
    if (type == PeFileType::Data && !size)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                      << ErrorString{"Invalid file size."});
    }

    if (type == PeFileType::Image && !size)
    {
      try
      {
        Module const module{process, reinterpret_cast<HMODULE>(address)};
        size_ = module.GetSize();
      }
      catch (...)
      {
        auto const region_alloc_size =
          detail::GetRegionAllocSize(*process_, base_);
        HADESMEM_DETAIL_ASSERT(region_alloc_size <
                               (std::numeric_limits<DWORD>::max)());
        size_ = static_cast<DWORD>(region_alloc_size);
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
// TODO: Consider if RvaToVa should be RvaToFilePtr, and RvaToVa should return
// the VA if the file were to be mapped.
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

  if (type == PeFileType::Data)
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

    // Windows will load specially crafted images with no sections.
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
        // scenario and then use PeFileType::Image.
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
  else if (type == PeFileType::Image)
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

  if (type == PeFileType::Data)
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
  else if (type == PeFileType::Image)
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
