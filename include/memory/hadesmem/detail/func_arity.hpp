// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "hadesmem/config.hpp"

namespace hadesmem
{

namespace detail
{

#ifndef HADESMEM_NO_VARIADIC_TEMPLATES

template <typename FuncT>
struct FuncArity
{ };

template <typename R, typename... Args>
struct FuncArity<R (*)(Args...)>
{
  static std::size_t const value = sizeof...(Args);
};

#else // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES
  
template <typename FuncT>
struct FuncArity
{ };

template <typename R>
struct FuncArity<R (*)()>
{
  static std::size_t const value = 0;
};

template <typename R, typename A0>
struct FuncArity<R (*)(A0)>
{
  static std::size_t const value = 1;
};

template <typename R, typename A0, typename A1>
struct FuncArity<R (*)(A0, A1)>
{
  static std::size_t const value = 2;
};

template <typename R, typename A0, typename A1, typename A2>
struct FuncArity<R (*)(A0, A1, A2)>
{
  static std::size_t const value = 3;
};

template <typename R, typename A0, typename A1, typename A2, typename A3>
struct FuncArity<R (*)(A0, A1, A2, A3)>
{
  static std::size_t const value = 4;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4>
struct FuncArity<R (*)(A0, A1, A2, A3, A4)>
{
  static std::size_t const value = 5;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5>
struct FuncArity<R (*)(A0, A1, A2, A3, A4, A5)>
{
  static std::size_t const value = 6;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6>
struct FuncArity<R (*)(A0, A1, A2, A3, A4, A5, A6)>
{
  static std::size_t const value = 7;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6, typename A7>
struct FuncArity<R (*)(A0, A1, A2, A3, A4, A5, A6, A7)>
{
  static std::size_t const value = 8;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6, typename A7, typename A8>
struct FuncArity<R (*)(A0, A1, A2, A3, A4, A5, A6, A7, A8)>
{
  static std::size_t const value = 9;
};

template <typename R, typename A0, typename A1, typename A2, typename A3, 
  typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct FuncArity<R (*)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9)>
{
  static std::size_t const value = 10;
};

#endif // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES

}

}
