// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/mpl/vector.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/config.hpp"

namespace hadesmem
{

namespace detail
{

#ifndef HADESMEM_NO_VARIADIC_TEMPLATES
 
template <typename FuncT>
struct FuncArgs
{ };

template <typename R, typename... Args>
struct FuncArgs<R (*)(Args...)>
{
  typedef boost::mpl::vector<Args...> type;
};

#else // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES
  
template <typename FuncT>
struct FuncArgs
{ };

template <typename R>
struct FuncArgs<R (*)()>
{ };

template <typename R, typename A0>
struct FuncArgs<R (*)(A0)>
{
  typedef boost::mpl::vector<A0> type;
};

template <typename R, typename A0, typename A1>
struct FuncArgs<R (*)(A0, A1)>
{
  typedef boost::mpl::vector<A0, A1> type;
};

template <typename R, typename A0, typename A1, typename A2>
struct FuncArgs<R (*)(A0, A1, A2)>
{
  typedef boost::mpl::vector<A0, A1, A2> type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3>
struct FuncArgs<R (*)(A0, A1, A2, A3)>
{
  typedef boost::mpl::vector<A0, A1, A2, A3> type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4>
struct FuncArgs<R (*)(A0, A1, A2, A3, A4)>
{
  typedef boost::mpl::vector<A0, A1, A2, A3, A4> type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5>
struct FuncArgs<R (*)(A0, A1, A2, A3, A4, A5)>
{
  typedef boost::mpl::vector<A0, A1, A2, A3, A4, A5> type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6>
struct FuncArgs<R (*)(A0, A1, A2, A3, A4, A5, A6)>
{
  typedef boost::mpl::vector<A0, A1, A2, A3, A4, A5, A6> type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6, typename A7>
struct FuncArgs<R (*)(A0, A1, A2, A3, A4, A5, A6, A7)>
{
  typedef boost::mpl::vector<A0, A1, A2, A3, A4, A5, A6, A7> type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6, typename A7, typename A8>
struct FuncArgs<R (*)(A0, A1, A2, A3, A4, A5, A6, A7, A8)>
{
  typedef boost::mpl::vector<A0, A1, A2, A3, A4, A5, A6, A7, A8> type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct FuncArgs<R (*)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9)>
{
  typedef boost::mpl::vector<A0, A1, A2, A3, A4, A5, A6, A7, A8, A9> type;
};

#endif // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES

}

}
