// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/mpl/vector.hpp>
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
struct FuncArgs
{ };

template <typename R, typename T, typename... Args>
struct FuncArgs<R (T::*)(Args...)>
{
  typedef boost::mpl::vector<T*, Args...> type;
};

template <typename R, typename T, typename... Args>
struct FuncArgs<R (T::*)(Args...) const>
{
  typedef boost::mpl::vector<T const*, Args...> type;
};

template <typename R, typename T, typename... Args>
struct FuncArgs<R (T::*)(Args...) volatile>
{
  typedef boost::mpl::vector<T volatile*, Args...> type;
};

template <typename R, typename T, typename... Args>
struct FuncArgs<R (T::*)(Args...) const volatile>
{
  typedef boost::mpl::vector<T const volatile*, Args...> type;
};

#if defined(HADESMEM_ARCH_X64)

template <typename R, typename... Args>
struct FuncArgs<R (*)(Args...)>
{
  typedef boost::mpl::vector<Args...> type;
};

#elif defined(HADESMEM_ARCH_X86)

template <typename R, typename... Args>
struct FuncArgs<R (__cdecl*)(Args...)>
{
  typedef boost::mpl::vector<Args...> type;
};

template <typename R, typename... Args>
struct FuncArgs<R (__stdcall*)(Args...)>
{
  typedef boost::mpl::vector<Args...> type;
};

template <typename R, typename... Args>
struct FuncArgs<R (__fastcall*)(Args...)>
{
  typedef boost::mpl::vector<Args...> type;
};

#endif

#else // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES
  
HADESMEM_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_REPEAT);

HADESMEM_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_ITERATION);

template <typename FuncT>
struct FuncArgs
{ };

#define BOOST_PP_LOCAL_MACRO(n) \
  template <typename R, typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArgs<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef boost::mpl::vector<T* BOOST_PP_ENUM_TRAILING_PARAMS(n, T)> type; \
}; \
\
template <typename R, typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArgs<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T)) const> \
{ \
  typedef boost::mpl::vector<T const* BOOST_PP_ENUM_TRAILING_PARAMS(n, T)> type; \
}; \
\
template <typename R, typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArgs<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T)) volatile> \
{ \
  typedef boost::mpl::vector<T volatile* BOOST_PP_ENUM_TRAILING_PARAMS(n, T)> type; \
}; \
\
template <typename R, typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArgs<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T)) const volatile> \
{ \
  typedef boost::mpl::vector<T const volatile* BOOST_PP_ENUM_TRAILING_PARAMS(n, T)> type; \
}; \

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_ARCH_X64)

#define BOOST_PP_LOCAL_MACRO(n) \
template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArgs<R (*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(n, T)> type; \
}; \

#elif defined(HADESMEM_ARCH_X86)

#define BOOST_PP_LOCAL_MACRO(n) \
  template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArgs<R (__cdecl*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(n, T)> type; \
}; \
\
template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArgs<R (__stdcall*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(n, T)> type; \
}; \
\
template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArgs<R (__fastcall*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(n, T)> type; \
}; \

#endif

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#include BOOST_PP_LOCAL_ITERATE()

#endif // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES

}

}
