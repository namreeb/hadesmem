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

// TODO: Add a new 'hostile' flag for things that are not just suspicious, but
// are actively hostile and never found in 'legitimate' modules, like the TLS
// AOI trick?
enum class WarningType
{
  kSuspicious,
  kUnsupported,
  kAll = -1
};

void SetCurrentFilePath(std::wstring const& path);

void WarnForCurrentFile(WarningType warned_type);

void ClearWarnForCurrentFile();
