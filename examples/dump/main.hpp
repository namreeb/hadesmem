// Copyright (C) 2010-2014 Joshua Boyce.
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

bool ConvertTimeStamp(std::time_t time, std::wstring& str);

template <typename CharT>
typename std::basic_string<CharT>::size_type
  FindFirstUnprintableClassicLocale(std::basic_string<CharT> const& s)
{
  auto const i =
    std::find_if(std::begin(s),
                 std::end(s),
                 [](CharT c)
                 { return !std::isprint(c, std::locale::classic()); });
  return i == std::end(s) ? std::basic_string<CharT>::npos
                          : std::distance(std::begin(s), i);
}
