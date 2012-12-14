// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>

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
struct FuncArity
{ };

template <typename R, typename T, typename... Args>
struct FuncArity<R (T::*)(Args...)>
{
  static std::size_t const value = sizeof...(Args) + 1;
};

template <typename R, typename T, typename... Args>
struct FuncArity<R (T::*)(Args...) const>
{
  static std::size_t const value = sizeof...(Args) + 1;
};

template <typename R, typename T, typename... Args>
struct FuncArity<R (T::*)(Args...) volatile>
{
  static std::size_t const value = sizeof...(Args) + 1;
};

template <typename R, typename T, typename... Args>
struct FuncArity<R (T::*)(Args...) const volatile>
{
  static std::size_t const value = sizeof...(Args) + 1;
};

#if defined(HADESMEM_ARCH_X64)

template <typename R, typename... Args>
struct FuncArity<R (*)(Args...)>
{
  static std::size_t const value = sizeof...(Args);
};

#elif defined(HADESMEM_ARCH_X86)

template <typename R, typename... Args>
struct FuncArity<R (__cdecl*)(Args...)>
{
  static std::size_t const value = sizeof...(Args);
};

template <typename R, typename... Args>
struct FuncArity<R (__stdcall*)(Args...)>
{
  static std::size_t const value = sizeof...(Args);
};

template <typename R, typename... Args>
struct FuncArity<R (__fastcall*)(Args...)>
{
  static std::size_t const value = sizeof...(Args);
};

#endif

#else // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES
  
HADESMEM_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_REPEAT);

HADESMEM_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_ITERATION);

template <typename FuncT>
struct FuncArity
{ };

#define BOOST_PP_LOCAL_MACRO(n) \
template <typename R, typename T \
  BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArity<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  static std::size_t const value = n + 1; \
}; \
\
template <typename R, typename T \
  BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArity<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T)) const> \
{ \
  static std::size_t const value = n + 1; \
}; \
\
template <typename R, typename T \
  BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArity<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T)) volatile> \
{ \
  static std::size_t const value = n + 1; \
}; \
\
template <typename R, typename T \
  BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArity<R (T::*)(BOOST_PP_ENUM_PARAMS(n, T)) const volatile> \
{ \
  static std::size_t const value = n + 1; \
}; \
\

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_ARCH_X64)

#define BOOST_PP_LOCAL_MACRO(n) \
template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArity<R (*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  static std::size_t const value = n; \
}; \

#elif defined(HADESMEM_ARCH_X86)

#define BOOST_PP_LOCAL_MACRO(n) \
  template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArity<R (__cdecl*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  static std::size_t const value = n; \
}; \
\
template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArity<R (__stdcall*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  static std::size_t const value = n; \
}; \
\
template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)> \
struct FuncArity<R (__fastcall*)(BOOST_PP_ENUM_PARAMS(n, T))> \
{ \
  static std::size_t const value = n; \
}; \

#endif

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#include BOOST_PP_LOCAL_ITERATE()

#endif // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES

}

}
