// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <type_traits>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/winternl.hpp>

namespace hadesmem
{
namespace detail
{
template <typename T, typename U = std::remove_cv_t<T>> struct IsCharType
{
  static bool const value =
    std::is_same<U, char>::value || std::is_same<U, signed char>::value ||
    std::is_same<U, unsigned char>::value || std::is_same<U, wchar_t>::value;
};

// TODO: Remove this now that we're using Dev14.
template <typename T> struct IsTriviallyCopyable
{
  // std::is_trivially_copyable is broken for arrays with Dev12.
  // https://connect.microsoft.com/VisualStudio/feedback/details/806233
  static bool const value = std::is_trivial<T>::value;
};

// TODO: Add support to traits for varargs functions (call conv is ignored on
// varargs). Remember though that you can have args before the elipsis, so you
// will need something like: struct FuncResult<R(C::*)(Args......)>

// TODO: Fix to support references properly etc.

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

#if !defined(HADESMEM_DETAIL_NO_VECTORCALL) && ((_MANAGED != 1) && (_M_CEE != 1))

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

#if !defined(HADESMEM_DETAIL_NO_VECTORCALL) && ((_MANAGED != 1) && (_M_CEE != 1))

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

#if !defined(HADESMEM_DETAIL_NO_VECTORCALL) && ((_MANAGED != 1) && (_M_CEE != 1))

HADESMEM_DETAIL_MAKE_FUNC_ARGS(__vectorcall)

#endif

template <typename FuncT> using FuncArgsT = typename FuncArgs<FuncT>::type;

template <typename FuncT> struct IsFunction
{
#pragma warning(push)
#pragma warning(disable : 6285)
  static bool const value =
    std::is_member_function_pointer<FuncT>::value ||
    std::is_function<FuncT>::value ||
    std::is_function<std::remove_pointer_t<FuncT>>::value;
#pragma warning(pop)
};
}
}
