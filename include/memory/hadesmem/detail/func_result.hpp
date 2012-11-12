// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include "hadesmem/config.hpp"

namespace hadesmem
{

namespace detail
{

#ifndef HADESMEM_NO_VARIADIC_TEMPLATES

template <typename FuncT>
struct FuncResult
{ };

template <typename R, typename... Args>
struct FuncResult<R (*)(Args...)>
{
  typedef R type;
};

#else // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES
  
template <typename FuncT>
struct FuncResult
{ };

template <typename R>
struct FuncResult<R (*)()>
{
  typedef R type;
};

template <typename R, typename A0>
struct FuncResult<R (*)(A0)>
{
  typedef R type;
};

template <typename R, typename A0, typename A1>
struct FuncResult<R (*)(A0, A1)>
{
  typedef R type;
};

template <typename R, typename A0, typename A1, typename A2>
struct FuncResult<R (*)(A0, A1, A2)>
{
  typedef R type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3>
struct FuncResult<R (*)(A0, A1, A2, A3)>
{
  typedef R type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4>
struct FuncResult<R (*)(A0, A1, A2, A3, A4)>
{
  typedef R type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5>
struct FuncResult<R (*)(A0, A1, A2, A3, A4, A5)>
{
  typedef R type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6>
struct FuncResult<R (*)(A0, A1, A2, A3, A4, A5, A6)>
{
  typedef R type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6, typename A7>
struct FuncResult<R (*)(A0, A1, A2, A3, A4, A5, A6, A7)>
{
  typedef R type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6, typename A7, typename A8>
struct FuncResult<R (*)(A0, A1, A2, A3, A4, A5, A6, A7, A8)>
{
  typedef R type;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct FuncResult<R (*)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9)>
{
  typedef R type;
};

#endif // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES

}

}
