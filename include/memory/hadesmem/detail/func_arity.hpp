// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/static_assert.hpp>

namespace hadesmem
{

namespace detail
{

template <typename FuncT>
struct FuncArity;

template <typename R, typename... Args>
struct FuncArity<R (Args...)>
{
  static std::size_t const value = sizeof...(Args);
};

}

}
