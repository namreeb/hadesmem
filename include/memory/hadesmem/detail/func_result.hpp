// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/preprocessor.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/config.hpp"
#include "hadesmem/detail/static_assert.hpp"

namespace hadesmem
{

namespace detail
{

#ifndef HADESMEM_NO_VARIADIC_TEMPLATES

template <typename FuncT>
struct FuncResult
{ };

template <typename R, typename T, typename... Args>
struct FuncResult<R (T::*)(Args...)>
{
  typedef R type;
};

template <typename R, typename T, typename... Args>
struct FuncResult<R (T::*)(Args...) const>
{
  typedef R type;
};

template <typename R, typename T, typename... Args>
struct FuncResult<R (T::*)(Args...) volatile>
{
  typedef R type;
};

template <typename R, typename T, typename... Args>
struct FuncResult<R (T::*)(Args...) const volatile>
{
  typedef R type;
};

#if defined(HADESMEM_ARCH_X64)

template <typename R, typename... Args>
struct FuncResult<R (*)(Args...)>
{
  typedef R type;
};

#elif defined(HADESMEM_ARCH_X86)

template <typename R, typename... Args>
struct FuncResult<R (__cdecl*)(Args...)>
{
  typedef R type;
};

template <typename R, typename... Args>
struct FuncResult<R (__stdcall*)(Args...)>
{
  typedef R type;
};

template <typename R, typename... Args>
struct FuncResult<R (__fastcall*)(Args...)>
{
  typedef R type;
};

#endif

#else // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES
  
HADESMEM_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_REPEAT);

HADESMEM_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_ITERATION);

template <typename FuncT>
struct FuncResult
{ };

#define BOOST_PP_LOCAL_MACRO(n) \
template <typename R, typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncResult<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef R type; \
}; \
\
template <typename R, typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncResult<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T)) const> \
{ \
  typedef R type; \
}; \
\
template <typename R, typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncResult<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T)) volatile> \
{ \
  typedef R type; \
}; \
\
template <typename R, typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncResult<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T)) const volatile> \
{ \
  typedef R type; \
}; \
\

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_ARCH_X64)

#define BOOST_PP_LOCAL_MACRO(n) \
template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncResult<R (*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef R type; \
}; \

#elif defined(HADESMEM_ARCH_X86)

#define BOOST_PP_LOCAL_MACRO(n) \
template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncResult<R (__cdecl*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef R type; \
}; \
\
template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncResult<R (__stdcall*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef R type; \
}; \
\
template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncResult<R (__fastcall*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef R type; \
}; \

#endif

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#include BOOST_PP_LOCAL_ITERATE()

#endif // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES

}

}
