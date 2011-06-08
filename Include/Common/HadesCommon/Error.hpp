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
#include <stdexcept>

// Boost
#include <boost/exception/all.hpp>

// Windows
#include <Windows.h>

namespace Hades
{
  // Base exception class
  class HadesError : public virtual std::exception, 
    public virtual boost::exception
  { };
  
  // Error info (function name)
  typedef boost::error_info<struct TagErrorFunc, std::string> ErrorFunction;
  // Error info (error string)
  typedef boost::error_info<struct TagErrorString, std::string> ErrorString;
  // Error info (Windows return error code)
  typedef boost::error_info<struct TagErrorCodeWinRet, DWORD_PTR> 
    ErrorCodeWinRet;
  // Error info (Windows last thread error code)
  typedef boost::error_info<struct TagErrorCodeWinLast, DWORD> 
    ErrorCodeWinLast;
  // Error info (Windows other error code)
  typedef boost::error_info<struct TagErrorCodeWinOther, DWORD_PTR> 
    ErrorCodeWinOther;
  // Error info (Other error code)
  typedef boost::error_info<struct TagErrorCodeOther, DWORD_PTR> 
    ErrorCodeOther;
}
