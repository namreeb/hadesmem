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

  WriteNewline(out);
  WriteNamedHex(out, L"Overlay Offset", overlay->GetOffset(), 1);
  WriteNamedHex(out, L"Overlay Size", overlay->GetSize(), 1);
  // TODO: Support dumping overlay to file.
}
