// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "relocations.hpp"

#include <iostream>

#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/relocations_dir.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

#include "main.hpp"

// TODO: Make this a proper API as part of PeLib.
void DumpRelocations(hadesmem::Process const& process,
                     hadesmem::PeFile const& pe_file)
{
  std::unique_ptr<hadesmem::RelocationsDir> relocations_dir;
  try
  {
    relocations_dir =
      std::make_unique<hadesmem::RelocationsDir>(process, pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    return;
  }

  std::wostream& out = std::wcout;

  WriteNewline(out);

  // TODO: Improve this.
  if (relocations_dir->IsInvalid())
  {
    WriteNormal(out, L"WARNING! Detected invalid relocation block(s). Output may be partially or entirely incorrect.", 2);
    WarnForCurrentFile(WarningType::kUnsupported);
  }

  auto const reloc_blocks = relocations_dir->GetRelocBlocks();
  HADESMEM_DETAIL_ASSERT(!reloc_blocks.empty());

  WriteNormal(out, L"Relocation Blocks:", 1);

  for (auto block : reloc_blocks)
  {
    WriteNewline(out);

    WriteNamedHex(out, L"VirtualAddress", block.va, 2);
    WriteNamedHex(out, L"SizeOfBlock", block.size, 2);

    WriteNewline(out);

    WriteNormal(out, L"Relocations:", 2);

    auto const& relocs = block.relocs;
    for (auto const reloc : relocs)
    {
      WriteNewline(out);

      WriteNamedHex(out, L"Type", reloc.type, 3);
      WriteNamedHex(out, L"Offset", reloc.offset, 3);

      if (reloc.type > 10)
      {
        WriteNormal(out, L"WARNING! Unknown relocation type.", 2);
        WarnForCurrentFile(WarningType::kUnsupported);
      }
    }
  }
}
