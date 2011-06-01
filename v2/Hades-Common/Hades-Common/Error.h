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

// Windows
#include <Windows.h>

// C++ Standard Library
#include <string>
#include <stdexcept>

// Boost
#pragma warning(push, 1)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <boost/exception/all.hpp>
#pragma warning(pop)

namespace Hades
{
  // Error info (function name)
  typedef boost::error_info<struct TagErrorFunc, std::string> ErrorFunction;
  // Error info (error string)
  typedef boost::error_info<struct TagErrorString, std::string> ErrorString;
  // Error info (Windows error code)
  typedef boost::error_info<struct TagErrorCodeWin, DWORD> ErrorCodeWin;

  // Base exception class
  class HadesError : public virtual std::exception, 
    public virtual boost::exception
  { };
}
