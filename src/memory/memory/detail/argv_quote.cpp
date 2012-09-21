// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/argv_quote.hpp"

#include <iterator>

namespace hadesmem
{

namespace detail
{

void ArgvQuote(std::wstring* command_line, std::wstring const& argument, 
  bool force)
{
  // Unless we're told otherwise, don't quote unless we actually
  // need to do so --- hopefully avoid problems if programs won't
  // parse quotes properly
  if (!force && !argument.empty() && argument.find_first_of(L" \t\n\v\"") 
    == argument.npos)
  {
    command_line->append(argument);
  }
  else 
  {
    command_line->push_back(L'"');

    for (auto it = std::begin(argument); ;++it)
    {
      int num_backslashes = 0;

      while (it != std::end(argument) && *it == L'\\') 
      {
        ++it;
        ++num_backslashes;
      }

      if (it == std::end(argument))
      {
        // Escape all backslashes, but let the terminating
        // double quotation mark we add below be interpreted
        // as a metacharacter.
        command_line->append(num_backslashes * 2, L'\\');
        break;
      }
      else if (*it == L'"')
      {
        // Escape all backslashes and the following
        // double quotation mark.
        command_line->append(num_backslashes * 2 + 1, L'\\');
        command_line->push_back(*it);
      }
      else
      {
        // Backslashes aren't special here.
        command_line->append(num_backslashes, L'\\');
        command_line->push_back(*it);
      }
    }

    command_line->push_back(L'"');
  }
}

}

}

