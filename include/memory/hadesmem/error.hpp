// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <exception>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/exception/all.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

namespace hadesmem
{

class Error : public virtual std::exception, 
  public virtual boost::exception
{
private:
  // This function exists to 'anchor' the class, and stop the compiler from 
  // copying vtable and RTTI info into every object file that includes 
  // this header.
  virtual void Anchor() const;
};

typedef boost::error_info<struct TagErrorString, std::string> ErrorString;
typedef boost::error_info<struct TagErrorCodeWinRet, DWORD_PTR> 
  ErrorCodeWinRet;
typedef boost::error_info<struct TagErrorCodeWinLast, DWORD> ErrorCodeWinLast;
typedef boost::error_info<struct TagErrorCodeWinOther, DWORD_PTR> 
  ErrorCodeWinOther;
typedef boost::error_info<struct TagErrorCodeOther, DWORD_PTR> ErrorCodeOther;

}
