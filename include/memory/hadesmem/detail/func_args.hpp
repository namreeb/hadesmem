// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/mpl/vector.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/static_assert.hpp>

namespace hadesmem
{

namespace detail
{

template <typename FuncT>
struct FuncArgs;
 
template <typename R, typename... Args>
struct FuncArgs<R (Args...)>
{
  // Using Boost.MPL because Intel C++ (2013, Update 1) chokes when using 
  // std::tuple and MSVC 2012 uses faux variadics, so require a preprocessor 
  // definition to increase the number of specializations.
  // TODO: Find a better solution to this.
  typedef boost::mpl::vector<Args...> type;
};

}

}
