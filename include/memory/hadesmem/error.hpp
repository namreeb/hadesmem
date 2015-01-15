// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <exception>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/exception/all.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>

namespace hadesmem
{
class Error : public virtual std::exception, public virtual boost::exception
{
};

using ErrorString = boost::error_info<struct TagErrorString, std::string>;
using ErrorCodeWinRet = boost::error_info<struct TagErrorCodeWinRet, DWORD_PTR>;
using ErrorCodeWinLast = boost::error_info<struct TagErrorCodeWinLast, DWORD>;
using ErrorCodeWinOther =
  boost::error_info<struct TagErrorCodeWinOther, DWORD_PTR>;
using ErrorCodeOther = boost::error_info<struct TagErrorCodeOther, DWORD_PTR>;
using ErrorCodeWinHr = boost::error_info<struct TagErrorCodWinHr, HRESULT>;
using ErrorCodeWinStatus =
  boost::error_info<struct TagErrorCodeWinStatus, NTSTATUS>;
using ErrorStringOther =
  boost::error_info<struct TagErrorStringOther, std::string>;
}

#define HADESMEM_DETAIL_THROW_EXCEPTION(x) BOOST_THROW_EXCEPTION(x)
