// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "strings.hpp"

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

// TODO: Make this less slow...
// TODO: Try and get the same results as Sysinternals Strings.
template <typename CharT>
void DumpStringsImpl(hadesmem::Process const& process, void* beg, void* end)
{
  std::wostream& out = std::wcout;

  std::uint8_t* current = static_cast<std::uint8_t*>(beg);
  std::basic_string<CharT> buf;
  do
  {
    hadesmem::ReadStringBounded<CharT>(
      process, current, std::back_inserter(buf), end);
    auto const skip_len = (buf.size() + 1) * sizeof(CharT);

    std::basic_string<CharT>::size_type unprint;
    do
    {
      if (buf.size() < 3)
      {
        break;
      }

      unprint = FindFirstUnprintableClassicLocale(buf);
      if (unprint == 0)
      {
        buf.erase(std::begin(buf));
      }
      else
      {
        if (unprint >= 3)
        {
          bool lossy = false;
          std::string const temp = hadesmem::detail::WideCharToMultiByte(
            buf.substr(0, unprint), &lossy);
          // TODO: Fix this... Currently we're throwing away anything that
          // contains a character which is not representable in the OEMCP, which
          // is clearly not what we want. Especially when dealing with Unicode
          // strings. This means that in the case where a legitimate string has
          // a bit of data before it that passes the isprint check but fails
          // this check, we miss the entire thing!
          if (!lossy)
          {
            WriteNamedNormal(out, L"String", temp.c_str(), 2);
          }
        }

        if (unprint != std::basic_string<CharT>::npos)
        {
          buf.erase(std::begin(buf), std::begin(buf) + unprint);
        }
      }
    } while (unprint != std::basic_string<CharT>::npos);

    current += skip_len;
    buf.clear();
  } while (current < end);
}
}

void DumpStrings(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file)
{
  std::wostream& out = std::wcout;

  std::uint8_t* const file_beg = static_cast<std::uint8_t*>(pe_file.GetBase());
  void* const file_end = file_beg + pe_file.GetSize();

  WriteNewline(out);
  WriteNormal(out, L"ASCII Strings:", 1);
  WriteNewline(out);
  DumpStringsImpl<char>(process, file_beg, file_end);

  // TODO: Fix Unicode string handling so we don't need two separate passes.

  WriteNewline(out);
  WriteNormal(out, L"Unicode Strings (Pass 1):", 1);
  WriteNewline(out);
  DumpStringsImpl<wchar_t>(process, file_beg, file_end);

  WriteNewline(out);
  WriteNormal(out, L"Unicode Strings (Pass 2):", 1);
  WriteNewline(out);
  DumpStringsImpl<wchar_t>(process, file_beg + 1, file_end);
}
