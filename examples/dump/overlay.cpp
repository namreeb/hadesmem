// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "sections.hpp"

#include <iostream>
#include <iterator>

#include <hadesmem/pelib/overlay.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"
#include "print.hpp"
#include "warning.hpp"

void DumpOverlay(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file)
{
  std::wostream& out = GetOutputStreamW();

  std::unique_ptr<hadesmem::Overlay const> overlay;
  try
  {
    overlay = std::make_unique<hadesmem::Overlay const>(process, pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    return;
  }

  auto const overlay_offset = reinterpret_cast<ULONG_PTR>(overlay->GetBase()) -
                              reinterpret_cast<ULONG_PTR>(pe_file.GetBase());
  WriteNewline(out);
  WriteNamedHex(out, L"Overlay Offset", overlay_offset, 1);
  // TODO: Support dumping overlay to file.
  auto const overlay_buffer = overlay->Get();
  WriteNamedHexContainer(out, L"Overlay", overlay_buffer, 1);
}
