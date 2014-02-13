// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <cstdint>

namespace hadesmem
{

class Process;
class PeFile;
}

void DisassembleEp(hadesmem::Process const& process,
                   hadesmem::PeFile const& pe_file,
                   std::uintptr_t ep_rva,
                   void* ep_va,
                   std::size_t tabs);
