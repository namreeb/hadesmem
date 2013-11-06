// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <tuple>

namespace hadesmem
{

namespace detail
{

template <typename FuncT>
struct FuncArgs;
 
template <typename R, typename... Args>
struct FuncArgs<R (Args...)>
{
  typedef std::tuple<Args...> type;
};

}

}
