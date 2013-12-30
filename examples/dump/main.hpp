// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>

namespace hadesmem
{
class Process;
class PeFile;
}

void DumpPeFile(hadesmem::Process const& process,
                hadesmem::PeFile const& pe_file,
                std::wstring const& path);

enum class WarningType
{
  kSuspicious, 
  kUnsupported,
  kAll = -1
};

void WarnForCurrentFile(WarningType warned_type);

void ClearWarnForCurrentFile();
