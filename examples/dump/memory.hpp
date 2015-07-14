// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

namespace hadesmem
{
class Process;
}

void DumpMemory(hadesmem::Process const& process, bool continue_on_error);
