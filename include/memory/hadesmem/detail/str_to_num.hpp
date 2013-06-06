// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <limits>
#include <string>
#include <sstream>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>

namespace hadesmem
{

namespace detail
{

// TODO: Better type checking/constraints. Improve genericity.
template <typename T, typename CharT>
T StrToUnsigned(std::basic_string<CharT> const& str)
{
  // TODO: Remove this workaround once Clang on Windows supports a newer 
  // STL implementation.
#if defined(HADESMEM_CLANG)
  std::basic_ostringstream converter(str);
  unsigned long long out = 0;
  if (!converter || (!converter >> out))
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Conversion failed."));
  }
#else // #if defined(HADESMEM_CLANG)
  unsigned long long const out = std::stoull(str);
#endif // #if defined(HADESMEM_CLANG)
  HADESMEM_ASSERT(out < (std::numeric_limits<T>::max)());
  return static_cast<T>(out);
}

}

}
