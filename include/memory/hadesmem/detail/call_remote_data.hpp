// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <type_traits>

#include <windows.h>

#include "hadesmem/detail/static_assert.hpp"

namespace hadesmem
{

namespace detail
{

struct CallResultRemote
{
  DWORD_PTR return_value;
  DWORD32 return_value_32;
  DWORD64 return_value_64;
  float return_value_float;
  double return_value_double;
  DWORD last_error;
};

// CallResultRemote must be POD because of 'offsetof' usage.
HADESMEM_STATIC_ASSERT(std::is_pod<CallResultRemote>::value);

}

}
