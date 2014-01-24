// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "relocations.hpp"

#include <iostream>

#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/relocation.hpp>
#include <hadesmem/pelib/relocation_list.hpp>
#include <hadesmem/pelib/relocation_block.hpp>
#include <hadesmem/pelib/relocation_block_list.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

#include "main.hpp"
#include "print.hpp"
#include "warning.hpp"

namespace
{

bool HasRelocationsDir(hadesmem::Process const& process,
                       hadesmem::PeFile const& pe_file)
{
  hadesmem::NtHeaders const nt_headers(process, pe_file);
  // Intentionally not checking whether the RVA or size is valid, because we
  // will detect an empty list in that case, at which point we want to warn.
  return (
    nt_headers.GetNumberOfRvaAndSizes() >
      static_cast<int>(hadesmem::PeDataDir::BaseReloc) &&
    nt_headers.GetDataDirectoryVirtualAddress(hadesmem::PeDataDir::BaseReloc));
}
}

void DumpRelocations(hadesmem::Process const& process,
                     hadesmem::PeFile const& pe_file)
{
  if (!HasRelocationsDir(process, pe_file))
  {
    return;
  }

  std::wostream& out = std::wcout;

  WriteNewline(out);

  hadesmem::RelocationBlockList reloc_blocks(process, pe_file);
  if (std::begin(reloc_blocks) != std::end(reloc_blocks))
  {
    WriteNormal(out, L"Relocation Blocks:", 1);
  }
  else
  {
    WriteNormal(out, L"WARNING! Relocation block list is invalid.", 1);
    WarnForCurrentFile(WarningType::kUnsupported);
  }

  for (auto block : reloc_blocks)
  {
    WriteNewline(out);

    auto const va = block.GetVirtualAddress();
    WriteNamedHex(out, L"VirtualAddress", va, 2);
    auto const size = block.GetSizeOfBlock();
    WriteNamedHex(out, L"SizeOfBlock", block.GetSizeOfBlock(), 2);

    WriteNewline(out);

    if (size)
    {
      WriteNormal(out, L"Relocations:", 2);
    }
    else
    {
      WriteNormal(out, L"WARNING! Detected zero sized relocation block.", 2);
      WarnForCurrentFile(WarningType::kUnsupported);
    }

    hadesmem::RelocationList relocs(process, pe_file, block.GetRelocationDataStart(), block.GetNumberOfRelocations());
    for (auto const reloc : relocs)
    {
      WriteNewline(out);

      auto const type = reloc.GetType();
      WriteNamedHex(out, L"Type", type, 3);
      WriteNamedHex(out, L"Offset", reloc.GetOffset(), 3);

      if (type > 10)
      {
        WriteNormal(out, L"WARNING! Unknown relocation type.", 3);
        WarnForCurrentFile(WarningType::kUnsupported);
      }
    }
  }
}
