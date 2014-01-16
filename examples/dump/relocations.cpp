// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "relocations.hpp"

#include <iostream>

#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

#include "main.hpp"

// TODO: Make this a proper API as part of PeLib.
void DumpRelocations(hadesmem::Process const& process,
                     hadesmem::PeFile const& pe_file)
{
  std::wostream& out = std::wcout;

  WriteNewline(out);

  hadesmem::NtHeaders const nt_headers(process, pe_file);
  if (static_cast<DWORD>(hadesmem::PeDataDir::BaseReloc) >=
      nt_headers.GetNumberOfRvaAndSizesClamped())
  {
    WriteNormal(out, L"WARNING! No relocation directory.", 1);
    WarnForCurrentFile(WarningType::kSuspicious);
    return;
  }
  DWORD const reloc_dir_rva =
    nt_headers.GetDataDirectoryVirtualAddress(hadesmem::PeDataDir::BaseReloc);
  if (!reloc_dir_rva)
  {
    WriteNormal(out, L"WARNING! No relocation directory.", 1);
    WarnForCurrentFile(WarningType::kSuspicious);
    return;
  }

  void* reloc_dir_va = hadesmem::RvaToVa(process, pe_file, reloc_dir_rva);
  if (!reloc_dir_va)
  {
    WriteNormal(out, L"WARNING! Invalid relocation directory RVA.", 1);
    WarnForCurrentFile(WarningType::kUnsupported);
    return;
  }

  DWORD const reloc_dir_size =
    nt_headers.GetDataDirectorySize(hadesmem::PeDataDir::BaseReloc);
  if (!reloc_dir_size)
  {
    WriteNormal(
      out,
      L"WARNING! Zero relocation directory size with a non-zero reloc dir VA.",
      1);
    WarnForCurrentFile(WarningType::kUnsupported);
    return;
  }

  void const* const reloc_dir_end =
    static_cast<std::uint8_t*>(reloc_dir_va) + reloc_dir_size;
  void const* const pe_file_end =
    static_cast<std::uint8_t*>(pe_file.GetBase()) + pe_file.GetSize();
  // TODO: Also fix this for images? Or is it discarded?
  if (pe_file.GetType() == hadesmem::PeFileType::Data &&
      reloc_dir_end > pe_file_end)
  {
    WriteNormal(out, L"WARNING! Relocation directory outside the file.", 1);
    WarnForCurrentFile(WarningType::kUnsupported);
    return;
  }

  WriteNormal(out, L"Relocation Blocks:", 1);

  while (reloc_dir_va < reloc_dir_end)
  {
    WriteNewline(out);

    auto const reloc_dir =
      hadesmem::Read<IMAGE_BASE_RELOCATION>(process, reloc_dir_va);
    WriteNamedHex(out, L"VirtualAddress", reloc_dir.VirtualAddress, 2);
    WriteNamedHex(out, L"SizeOfBlock", reloc_dir.SizeOfBlock, 2);

    WriteNewline(out);

    DWORD const num_relocs =
      reloc_dir.SizeOfBlock
        ? (reloc_dir.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD)
        : 0;
    PWORD reloc_data = reinterpret_cast<PWORD>(
      static_cast<IMAGE_BASE_RELOCATION*>(reloc_dir_va) + 1);
    void const* const reloc_data_end = reinterpret_cast<void const*>(
      reinterpret_cast<std::uintptr_t>(reloc_data) + num_relocs);
    if (reloc_data_end < reloc_data || reloc_data_end > reloc_dir_end)
    {
      WriteNormal(out,
                  L"WARNING! Relocation block is outside the file. Terminating "
                  L"enumeration early.",
                  2);
      WarnForCurrentFile(WarningType::kUnsupported);
      break;
    }

    WriteNormal(out, L"Relocations:", 2);

    WriteNewline(out);

    auto const relocs =
      hadesmem::ReadVector<WORD>(process, reloc_data, num_relocs);
    for (auto const reloc : relocs)
    {
      BYTE const type = reloc >> 12;
      WriteNamedHex(out, L"Type", type, 3);
      WORD const offset = reloc & 0xFFF;
      WriteNamedHex(out, L"Offset", offset, 3);

      switch (type)
      {
      case IMAGE_REL_BASED_ABSOLUTE:
      case IMAGE_REL_BASED_HIGH:
      case IMAGE_REL_BASED_LOW:
      case IMAGE_REL_BASED_HIGHLOW:
      case IMAGE_REL_BASED_HIGHADJ:
      case IMAGE_REL_BASED_MACHINE_SPECIFIC_5:
      case IMAGE_REL_BASED_RESERVED:
      case IMAGE_REL_BASED_MACHINE_SPECIFIC_7:
      case IMAGE_REL_BASED_MACHINE_SPECIFIC_8:
      case IMAGE_REL_BASED_MACHINE_SPECIFIC_9:
      case IMAGE_REL_BASED_DIR64:
        break;

      default:
        WriteNormal(out, L"WARNING! Unknown relocation type.", 2);
        WarnForCurrentFile(WarningType::kUnsupported);
      }
    }

    reloc_dir_va = reloc_data + num_relocs;
  }
}
