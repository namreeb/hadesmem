/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// C++ Standard Library
#include <string>

namespace HadesMem
{
  namespace Detail
  {
    // This routine appends the given argument to a command line such
    // that CommandLineToArgvW will return the argument string unchanged.
    // Arguments in a command line should be separated by spaces; this
    // function does not add these spaces.
    inline void ArgvQuote(const std::wstring& Argument, 
      std::wstring& CommandLine, bool Force)
    {
      // Unless we're told otherwise, don't quote unless we actually
      // need to do so --- hopefully avoid problems if programs won't
      // parse quotes properly
      if (!Force && !Argument.empty () && Argument.find_first_of(L" \t\n\v\"") 
        == Argument.npos)
      {
        CommandLine.append(Argument);
      }
      else 
      {
        CommandLine.push_back(L'"');
        
        for (auto It = Argument.begin(); ;++It)
        {
          unsigned NumberBackslashes = 0;
          
          while (It != Argument.end () && *It == L'\\') 
          {
            ++It;
            ++NumberBackslashes;
          }
      
          if (It == Argument.end ())
          {
            // Escape all backslashes, but let the terminating
            // double quotation mark we add below be interpreted
            // as a metacharacter.
            CommandLine.append(NumberBackslashes * 2, L'\\');
            break;
          }
          else if (*It == L'"')
          {
            // Escape all backslashes and the following
            // double quotation mark.
            CommandLine.append(NumberBackslashes * 2 + 1, L'\\');
            CommandLine.push_back(*It);
          }
          else
          {
            // Backslashes aren't special here.
            CommandLine.append(NumberBackslashes, L'\\');
            CommandLine.push_back(*It);
          }
        }
        
        CommandLine.push_back(L'"');
      }
    }
  }
}
