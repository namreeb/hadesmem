/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

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
#include <cctype>
#include <locale>
#include <iterator>
#include <algorithm>

// Boost
#include <boost/lexical_cast.hpp>

namespace Hades
{
  namespace Util
  {
    // Convert a wide string to a narrow string
    inline std::string ConvertStr(std::wstring const& Source)
    {
      auto const Loc(std::locale(""));
      auto const& MyCType(std::use_facet<std::ctype<wchar_t>>(Loc));

      std::string Dest;
      std::transform(Source.begin(), Source.end(), std::back_inserter(Dest), 
        [&] (wchar_t Current)
      {
        return MyCType.narrow(Current);
      });

      return Dest;
    }

    // Convert a narrow string to a wide string
    inline std::wstring ConvertStr(std::string const& Source)
    {
      auto const Loc(std::locale(""));
      auto const& MyCType(std::use_facet<std::ctype<char>>(Loc));

      std::wstring Dest;
      std::transform(Source.begin(), Source.end(), std::back_inserter(Dest), 
        [&] (char Current)
      {
        return MyCType.widen(Current);
      });

      return Dest;
    }
  }
}

// Boost.LexicalCast specialization to allow conversions from wide to narrow 
// strings. Thanks Xeno123 for the idea.
template<> 
inline std::string boost::lexical_cast<std::string, std::wstring>(
  std::wstring const& Source)
{
  return Hades::Util::ConvertStr(Source);
}

// Boost.LexicalCast specialization to allow conversions from narrow to wide 
// strings. Thanks Xeno123 for the idea.
template<> 
inline std::wstring boost::lexical_cast<std::wstring, std::string>(
  std::string const& Source)
{
  return Hades::Util::ConvertStr(Source);
}
