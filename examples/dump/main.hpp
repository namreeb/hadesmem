// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <locale>
#include <ostream>
#include <string>
#include <vector>

#include "warning.hpp"

// TODO: Clean up this header. It contains a lot of random code that doesn't
// belong here.

namespace hadesmem
{
class Process;
class PeFile;
}

void DumpPeFile(hadesmem::Process const& process,
                hadesmem::PeFile const& pe_file,
                std::wstring const& path);

void SetCurrentFilePath(std::wstring const& path);

void HandleLongOrUnprintableString(std::wstring const& name,
                                   std::wstring const& description,
                                   std::size_t tabs,
                                   WarningType warning_type,
                                   std::string value);

std::string::size_type FindFirstUnprintableClassicLocale(std::string const& s);

bool ConvertTimeStamp(std::time_t time, std::wstring& str);
