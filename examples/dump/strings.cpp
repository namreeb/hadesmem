// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "strings.hpp"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

#include "main.hpp"
#include "print.hpp"
#include "warning.hpp"

namespace
{
void DumpStringsImpl(hadesmem::Process const& /*process*/,
                     hadesmem::PeFile const& pe_file,
                     void* beg,
                     void* end,
                     bool wide)
{
  std::wostream& out = GetOutputStreamW();

  if (pe_file.GetType() != hadesmem::PeFileType::kData)
  {
    // TODO: Fix this.
    WriteNormal(out,
                L"WARNING! Skipping string dump (Image file type is currently "
                L"unsupported).",
                2);
    WarnForCurrentFile(WarningType::kUnsupported);
    return;
  }

  std::size_t const kMinStringLen = 5;

  std::string buf;
  std::locale const& loc = std::locale::classic();
  // TODO: Support actual wide strings, not just those with only ASCII
  // characters.
  auto const step = wide ? 2 : 1;
  for (std::uint8_t* current = static_cast<std::uint8_t*>(beg);
       (current + step - 1) < end;
       current += (wide ? 2 : 1))
  {
    // TODO: Detect and truncate extremely long strings (with a warning).
    bool const is_print = (wide ? *(current + 1) == 0 : true) &&
                          std::isprint(static_cast<char>(*current), loc);
    if (is_print)
    {
      buf += static_cast<char>(*current);
    }

    if (!is_print || current + (wide ? 2 : 1) == end)
    {
      if (buf.size() >= kMinStringLen)
      {
        // TODO: Option to log file offset (and rva?) of string.
        WriteNamedNormal(out, L"String", buf.c_str(), 2);
      }

      buf.clear();
    }
  }
}
}

void DumpStrings(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file)
{
  std::wostream& out = GetOutputStreamW();

  std::uint8_t* const file_beg = static_cast<std::uint8_t*>(pe_file.GetBase());
  void* const file_end = file_beg + pe_file.GetSize();

  // TODO: Fix to not require multiple passes.

  WriteNewline(out);
  WriteNormal(out, L"Narrow Strings:", 1);
  WriteNewline(out);
  DumpStringsImpl(process, pe_file, file_beg, file_end, false);

  WriteNewline(out);
  WriteNormal(out, L"Wide Strings (Pass 1):", 1);
  WriteNewline(out);
  DumpStringsImpl(process, pe_file, file_beg, file_end, true);

  WriteNewline(out);
  WriteNormal(out, L"Wide Strings (Pass 2):", 1);
  WriteNewline(out);
  DumpStringsImpl(process, pe_file, file_beg + 1, file_end, true);
}
