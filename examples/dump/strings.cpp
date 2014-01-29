// Copyright (C) 2010-2013 Joshua Boyce.
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
  std::wostream& out = std::wcout;

  // TODO: Fix this to support Images.
  if (pe_file.GetType() != hadesmem::PeFileType::Data)
  {
    WriteNormal(out,
                L"WARNING! Skipping string dump (Image file type is curerntly "
                L"unsupported).",
                2);
    WarnForCurrentFile(WarningType::kUnsupported);
    return;
  }

  std::size_t const kMinStringLen = 3;

  std::string buf;
  std::locale const& loc = std::locale::classic();
  for (std::uint8_t* current = static_cast<std::uint8_t*>(beg); current < end;
       current += (wide ? 2 : 1))
  {
    // TODO: Fix this to support actual Unicode strings, not just wide strings
    // with only ASCII characters.
    bool const is_print = (wide ? *(current + 1) == 0 : true) &&
                          std::isprint(static_cast<char>(*current), loc);
    if (is_print)
    {
      // TODO: Detect and truncate extremely long strings (with a warning).
      buf += static_cast<char>(*current);
    }

    if (!is_print || current + (wide ? 2 : 1) == end)
    {
      if (buf.size() >= kMinStringLen)
      {
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
  std::wostream& out = std::wcout;

  std::uint8_t* const file_beg = static_cast<std::uint8_t*>(pe_file.GetBase());
  void* const file_end = file_beg + pe_file.GetSize();

  // TODO: Fix this to not require multiple passes.

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
