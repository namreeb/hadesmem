// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <exception>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/exception/all.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>

namespace hadesmem
{

    // Header-only library unfortunately means no vtable/rtti anchor.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif // #if defined(HADESMEM_CLANG)

    class Error : public virtual std::exception,
        public virtual boost::exception
    { };

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)

    typedef boost::error_info<struct TagErrorString, std::string> ErrorString;
    typedef boost::error_info<struct TagErrorCodeWinRet, DWORD_PTR>
        ErrorCodeWinRet;
    typedef boost::error_info<struct TagErrorCodeWinLast, DWORD> ErrorCodeWinLast;
    typedef boost::error_info<struct TagErrorCodeWinOther, DWORD_PTR>
        ErrorCodeWinOther;
    typedef boost::error_info<struct TagErrorCodeOther, DWORD_PTR> ErrorCodeOther;
    typedef boost::error_info<struct TagErrorCodeOther, HRESULT> ErrorCodeWinHr;
    typedef boost::error_info<struct TagErrorString, std::string>
        ErrorStringOther;

}

#define HADESMEM_DETAIL_THROW_EXCEPTION(x) BOOST_THROW_EXCEPTION(x)
