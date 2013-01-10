// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <exception>

#include <windows.h>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/exception/all.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/config.hpp"

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

#define HADESMEM_THROW_EXCEPTION(x) BOOST_THROW_EXCEPTION(x)
