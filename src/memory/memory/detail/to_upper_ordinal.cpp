// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/to_upper_ordinal.hpp"

#include <limits>
#include <vector>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

#include "hadesmem/error.hpp"

namespace hadesmem
{

namespace detail
{

std::wstring ToUpperOrdinal(std::wstring const& str)
{
  std::vector<wchar_t> str_buf(std::begin(str), std::end(str));
  str_buf.push_back(0);
  
  BOOST_ASSERT(str_buf.size() < (std::numeric_limits<DWORD>::max)());
  DWORD const num_converted = ::CharUpperBuff(str_buf.data(), 
    static_cast<DWORD>(str_buf.size()));
  if (num_converted != str_buf.size())
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("CharUpperBuff failed.") << 
      ErrorCodeWinRet(num_converted) << 
      ErrorCodeWinLast(last_error));
  }
  
  return str_buf.data();
}

}

}
