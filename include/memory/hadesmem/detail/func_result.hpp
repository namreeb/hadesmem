// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

namespace hadesmem
{

namespace detail
{

template <typename FuncT>
struct FuncResult;

template <typename R, typename... Args>
struct FuncResult<R (Args...)>
{
  typedef R type;
};

}

}
