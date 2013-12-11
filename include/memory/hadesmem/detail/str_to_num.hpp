// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <limits>
#include <locale>
#include <sstream>
#include <string>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/error.hpp>

namespace hadesmem
{

namespace detail
{

template <typename T, typename CharT>
T StrToNum(std::basic_string<CharT> const& str)
{
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value);
  std::basic_ostringstream<CharT> converter(str);
  converter.imbue(std::locale::classic());
  T out = 0;
  if (!converter || (!converter >> out))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("Conversion failed."));
  }
  return out;
}
}
}
