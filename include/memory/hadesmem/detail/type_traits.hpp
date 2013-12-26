// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <type_traits>

#include <hadesmem/config.hpp>

namespace hadesmem
{

namespace detail
{

template <typename T, typename U = std::remove_cv_t<T>> struct IsCharType
{
  static bool const value =
    std::is_same<U, char>::value || std::is_same<U, signed char>::value ||
    std::is_same<U, unsigned char>::value || std::is_same<U, wchar_t>::value ||
    std::is_same<U, char16_t>::value || std::is_same<U, char32_t>::value;
};

template <typename T> struct IsTriviallyCopyable
{
  // TODO: Update to use std::is_trivially_copyable trait when
  // available in libstdc++.
  // TODO: Update to use std::is_trivially_copyable when MSVC is
  // fixed. Dev12 seems to be broken for arrays.
  static bool const value = std::is_trivial<T>::value;
};

// TODO: Add support to traits for varargs functions (call conv is
// ignored on varargs). Remember though that you can have args before
// the elipsis, so you will need something like:
// struct FuncResult<R(C::*)(Args......)>

template <typename FuncT> struct FuncResult;

template <typename C, typename R, typename... Args>
struct FuncResult<R (C::*)(Args...)>
{
  using type = R;
};

template <typename C, typename R, typename... Args>
struct FuncResult<R (C::*)(Args...) const>
{
  using type = R;
};

#define HADESMEM_DETAIL_MAKE_FUNC_RESULT(call_conv)                            \
  \
template<typename R,                                                           \
         typename... Args> struct FuncResult<R(call_conv*)(Args...)>           \
  \
{                                                                         \
    using type = R;                                                            \
  \
};                                                                             \
  \
template<typename R, typename... Args> struct FuncResult<R call_conv(Args...)> \
  \
{                                                                         \
    using type = R;                                                            \
  \
};

#if defined(HADESMEM_DETAIL_ARCH_X64)

template <typename R, typename... Args> struct FuncResult<R (*)(Args...)>
{
  using type = R;
};

template <typename R, typename... Args> struct FuncResult<R(Args...)>
{
  using type = R;
};

#elif defined(HADESMEM_DETAIL_ARCH_X86)

HADESMEM_DETAIL_MAKE_FUNC_RESULT(__cdecl)
HADESMEM_DETAIL_MAKE_FUNC_RESULT(__stdcall)
HADESMEM_DETAIL_MAKE_FUNC_RESULT(__fastcall)

#else
#error "[HadesMem] Unsupported architecture."
#endif

#if !defined(HADESMEM_DETAIL_NO_VECTORCALL)

HADESMEM_DETAIL_MAKE_FUNC_RESULT(__vectorcall)

#endif

template <typename FuncT> using FuncResultT = typename FuncResult<FuncT>::type;

template <typename FuncT> struct FuncArity;

template <typename C, typename R, typename... Args>
struct FuncArity<R (C::*)(Args...)>
{
  static std::size_t const value = sizeof...(Args) + 1;
};

template <typename C, typename R, typename... Args>
struct FuncArity<R (C::*)(Args...) const>
{
  static std::size_t const value = sizeof...(Args) + 1;
};

#define HADESMEM_DETAIL_MAKE_FUNC_ARITY(call_conv)                             \
  \
template<typename R,                                                           \
         typename... Args> struct FuncArity<R(call_conv*)(Args...)>            \
  \
{                                                                         \
    static std::size_t const value = sizeof...(Args);                          \
  \
};                                                                             \
  \
template<typename R, typename... Args> struct FuncArity<R call_conv(Args...)>  \
  \
{                                                                         \
    static std::size_t const value = sizeof...(Args);                          \
  \
};

#if defined(HADESMEM_DETAIL_ARCH_X64)

template <typename R, typename... Args> struct FuncArity<R (*)(Args...)>
{
  static std::size_t const value = sizeof...(Args);
};

template <typename R, typename... Args> struct FuncArity<R(Args...)>
{
  static std::size_t const value = sizeof...(Args);
};

#elif defined(HADESMEM_DETAIL_ARCH_X86)

HADESMEM_DETAIL_MAKE_FUNC_ARITY(__cdecl)
HADESMEM_DETAIL_MAKE_FUNC_ARITY(__stdcall)
HADESMEM_DETAIL_MAKE_FUNC_ARITY(__fastcall)

#else
#error "[HadesMem] Unsupported architecture."
#endif

#if !defined(HADESMEM_DETAIL_NO_VECTORCALL)

HADESMEM_DETAIL_MAKE_FUNC_ARITY(__vectorcall)

#endif

template <typename FuncT> struct FuncArgs;

template <typename C, typename R, typename... Args>
struct FuncArgs<R (C::*)(Args...)>
{
  using type = std::tuple<C*, Args...>;
};

template <typename C, typename R, typename... Args>
struct FuncArgs<R (C::*)(Args...) const>
{
  using type = std::tuple<C const*, Args...>;
};

#define HADESMEM_DETAIL_MAKE_FUNC_ARGS(call_conv)                              \
  \
template<typename R, typename... Args> struct FuncArgs<R(call_conv*)(Args...)> \
  \
{                                                                         \
    using type = std::tuple<Args...>;                                          \
  \
};                                                                             \
  \
template<typename R, typename... Args> struct FuncArgs<R call_conv(Args...)>   \
  \
{                                                                         \
    using type = std::tuple<Args...>;                                          \
  \
};

#if defined(HADESMEM_DETAIL_ARCH_X64)

template <typename R, typename... Args> struct FuncArgs<R (*)(Args...)>
{
  using type = std::tuple<Args...>;
};

template <typename R, typename... Args> struct FuncArgs<R(Args...)>
{
  using type = std::tuple<Args...>;
};

#elif defined(HADESMEM_DETAIL_ARCH_X86)

HADESMEM_DETAIL_MAKE_FUNC_ARGS(__cdecl)
HADESMEM_DETAIL_MAKE_FUNC_ARGS(__stdcall)
HADESMEM_DETAIL_MAKE_FUNC_ARGS(__fastcall)

#else
#error "[HadesMem] Unsupported architecture."
#endif

#if !defined(HADESMEM_DETAIL_NO_VECTORCALL)

HADESMEM_DETAIL_MAKE_FUNC_ARGS(__vectorcall)

#endif

template <typename FuncT> using FuncArgsT = typename FuncArgs<FuncT>::type;

// WARNING! Here be dragons... GCC sucks and doesn't properly
// distinguish between function pointers with different calling
// conventions. Depending on the ABI version you're using you either
// get a compiler error or a linker error if you try to overload
// based on the calling convention of a function pointer taken as
// a parameter. What's even worse is that this also applies to
// templates! So, in order to work around this I'm using a disgusting
// hack whereby I use templates to detect the calling convention (if
// any) of the type, which the compiler can then be used to
// distinguish the different overloads from each other.
// This is bad and GCC should feel bad...
// TODO: Find a less heinous workaround, or at the very least clean
// it up somehow to make it less disgusting at the spots where this
// has to be used.

struct DetailCallConv
{
  enum
  {
    kCdecl,
    kStdCall,
    kThisCall,
    kFastCall,
    kVectorCall,
    kX64
  };
};

template <typename FuncT> struct FuncCallConv
{
  static int const value = -1;
};

template <typename C, typename R, typename... Args>
struct FuncCallConv<R (C::*)(Args...)>
{
  static int const value = DetailCallConv::kThisCall;
};

template <typename C, typename R, typename... Args>
struct FuncCallConv<R (C::*)(Args...) const>
{
  static int const value = DetailCallConv::kThisCall;
};

#define HADESMEM_DETAIL_MAKE_FUNC_CALL_CONV(call_conv, call_conv_value)        \
  \
template<typename R,                                                           \
         typename... Args> struct FuncCallConv<R(call_conv*)(Args...)>         \
  \
{                                                                         \
    static int const value = call_conv_value;                                  \
  \
};                                                                             \
  \
template<typename R,                                                           \
         typename... Args> struct FuncCallConv<R call_conv(Args...)>           \
  \
{                                                                         \
    static int const value = call_conv_value;                                  \
  \
};

#if defined(HADESMEM_DETAIL_ARCH_X64)

template <typename R, typename... Args> struct FuncCallConv<R (*)(Args...)>
{
  static int const value = DetailCallConv::kX64;
};

template <typename R, typename... Args> struct FuncCallConv<R(Args...)>
{
  static int const value = DetailCallConv::kX64;
};

#elif defined(HADESMEM_DETAIL_ARCH_X86)

HADESMEM_DETAIL_MAKE_FUNC_CALL_CONV(__cdecl, DetailCallConv::kCdecl)
HADESMEM_DETAIL_MAKE_FUNC_CALL_CONV(__stdcall, DetailCallConv::kStdCall)
HADESMEM_DETAIL_MAKE_FUNC_CALL_CONV(__fastcall, DetailCallConv::kFastCall)

#else
#error "[HadesMem] Unsupported architecture."
#endif

#if !defined(HADESMEM_DETAIL_NO_VECTORCALL)

HADESMEM_DETAIL_MAKE_FUNC_CALL_CONV(__vectorcall, DetailCallConv::kVectorCall)

#endif
}
}
