// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

namespace hadesmem
{
class Process;
class PeFile;
}

void DumpOverlay(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file);
