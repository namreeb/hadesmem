// Copyright (C) 2010-2015 Joshua Boyce
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

// TODO: Remove Boost.Exception dependency.

// TODO: Add stack trace support (debug mode only?).

// TODO: Investigate all places we're currently catching exceptions, and
// redesign the APIs if it makes sense.

// TODO: Make it configurable whether or not all failures are errors or whether
// it is 'best effort' for things like module enumeration etc.

// TODO: Rethink our error handling strategy. We should only be throwing
// exceptions where a function can't perform its advertised duty.
// Decent idea here. http://bit.ly/1Nd8snv
// If a function is unable to meet its postconditions (as defined by its
// documented interface) due to reasons beyond its control, throw an exception.
// If the function has some possible result which might be considered an error
// but is a reasonably expected result of the interface, it should be some sort
// of error code. This may be passed back in a pair or tuple with the result, or
// a special value of the output, or use output reference parameters and return
// the code.
// For sanity checks within a single unit of code (that is, I know this
// condition should be true, so I've messed something up if it isn't), make
// assertions. Use static_assert when you can to catch problems at compile-time.

// TODO: Add non-throwing variants of API where it makes sense to do so (and
// implement the throwing variant in terms of the non-throwing variant of
// course, for performance reasons). To fix the "information loss" problem
// caused by only returning a bool instead of throwing an exception with full
// state, we should replace all exceptions with debug logging. Better for perf,
// and for debugging, because we will benefit from future improvements to the
// tracing library.

// TODO: Provide the strongest exception guarantees possible.
// http://bit.ly/1OwkkEe

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
