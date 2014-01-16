// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <array>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <vector>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{

// TODO: Fix the name of this.
// TODO: Port the actual enumeration to a RelocationsList instead, and make this
// class instead for operating on a single base reloc block at a time.
class RelocationsDir
{
public:
  explicit RelocationsDir(Process const& process, PeFile const& pe_file)
    : process_(&process),
      pe_file_(&pe_file),
      base_(nullptr),
      size_(0U),
      invalid_(false),
      data_()
  {
    NtHeaders const nt_headers(process, pe_file);

    // TODO: Some sort of API to handle this common case.
    DWORD const data_dir_va =
      nt_headers.GetDataDirectoryVirtualAddress(PeDataDir::BaseReloc);
    size_ = nt_headers.GetDataDirectorySize(PeDataDir::BaseReloc);
    if (!data_dir_va || !size_)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("PE file has no relocations directory."));
    }

    base_ = static_cast<PBYTE>(RvaToVa(process, pe_file, data_dir_va));
    if (!base_)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Relocations directory is invalid."));
    }

    // Cast to integer and back to avoid pointer overflow UB.
    auto const relocs_end = reinterpret_cast<void const*>(
      reinterpret_cast<std::uintptr_t>(base_) + size_);
    auto const file_end =
      static_cast<std::uint8_t*>(pe_file.GetBase()) + pe_file.GetSize();
    // TODO: Also fix this for images? Or is it discarded?
    // TODO: Fix this to handle files with 'virtual' relocs which will still be loaded by Windows.
    // Sample: virtrelocXP.exe
    if (pe_file.GetType() == hadesmem::PeFileType::Data &&
        (relocs_end < base_ || relocs_end > file_end))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Relocations directory is outside file."));
    }

    UpdateRead();
  }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  DWORD GetSize() const HADESMEM_DETAIL_NOEXCEPT
  {
    return size_;
  }

  bool IsInvalid() const HADESMEM_DETAIL_NOEXCEPT
  {
    return invalid_;
  }

  struct Reloc
  {
    BYTE type;
    WORD offset;
  };

  struct RelocBlock
  {
    DWORD va;
    DWORD size;
    std::vector<Reloc> relocs;
  };

  void UpdateRead()
  {
    std::vector<RelocBlock> reloc_blocks;

    void* current = base_;
    void const* end = static_cast<std::uint8_t*>(base_) + size_;
    do
    {
      auto const reloc_dir =
        hadesmem::Read<IMAGE_BASE_RELOCATION>(*process_, current);

      // TODO: Check whether this is even valid... Should this mark as invalid
      // instead of simply skipping?
      if (!reloc_dir.SizeOfBlock)
      {
        continue;
      }

      DWORD const num_relocs = static_cast<DWORD>(
        (reloc_dir.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD));
      PWORD reloc_data = reinterpret_cast<PWORD>(
        static_cast<IMAGE_BASE_RELOCATION*>(current) + 1);
      void const* const reloc_data_end = reinterpret_cast<void const*>(
        reinterpret_cast<std::uintptr_t>(reloc_data) + num_relocs);
      if (reloc_data_end < reloc_data || reloc_data_end > end)
      {
        invalid_ = true;
        break;
      }

      std::vector<Reloc> relocs;
      auto const relocs_raw =
        hadesmem::ReadVector<WORD>(*process_, reloc_data, num_relocs);
      for (auto const reloc : relocs_raw)
      {
        BYTE const type = static_cast<BYTE>(reloc >> 12);
        WORD const offset = reloc & 0x0FFF;
        relocs.emplace_back(Reloc{type, offset});
      }

      reloc_blocks.emplace_back(RelocBlock{
        reloc_dir.VirtualAddress, reloc_dir.SizeOfBlock, std::move(relocs)});

      current = reloc_data + num_relocs;
    } while (current < end);

    data_ = reloc_blocks;
  }

  // TODO: Add setters and UpdateWrite.

  std::vector<RelocBlock> GetRelocBlocks() const
  {
    return data_;
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  PBYTE base_;
  DWORD size_;
  bool invalid_;
  std::vector<RelocBlock> data_;
};

inline bool operator==(RelocationsDir const& lhs, RelocationsDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(RelocationsDir const& lhs, RelocationsDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(RelocationsDir const& lhs, RelocationsDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(RelocationsDir const& lhs, RelocationsDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(RelocationsDir const& lhs, RelocationsDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(RelocationsDir const& lhs, RelocationsDir const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, RelocationsDir const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, RelocationsDir const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << rhs.GetBase();
  lhs.imbue(old);
  return lhs;
}
}
