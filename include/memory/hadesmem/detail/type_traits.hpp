// Copyright (C) 2010-2014 Joshua Boyce.
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

template <typename T> struct IsTriviallyCopyable
{
  // std::is_trivially_copyable is unavailable in libstdc++ and broken in Dev12
  // (for arrays).
  static bool const value = std::is_trivial<T>::value;
};

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

template <typename FuncT, typename DetourT> struct DetourCallback;

template <typename C, typename R, typename... Args, typename DetourT>
struct DetourCallback<R (C::*)(Args...), DetourT>
{
  typedef R(type)(DetourT*, C*, Args...);
};

template <typename C, typename R, typename... Args, typename DetourT>
struct DetourCallback<R (C::*)(Args...) const, DetourT>
{
  typedef R(type)(DetourT*, C const*, Args...);
};

#define HADESMEM_DETAIL_MAKE_DETOUR_CALLBACK(call_conv)                        \
  \
template<typename R,                                                           \
         typename... Args,                                                     \
         typename DetourT> struct DetourCallback<R(call_conv*)(Args...),       \
                                                 DetourT>                      \
  \
{                                                                         \
    typedef R(type)(DetourT*, Args...);                                        \
  \
};                                                                             \
  \
template<typename R,                                                           \
         typename... Args,                                                     \
         typename DetourT> struct DetourCallback<R call_conv(Args...),         \
                                                 DetourT>                      \
  \
{                                                                         \
    typedef R(type)(DetourT*, Args...);                                        \
  \
};

#if defined(HADESMEM_DETAIL_ARCH_X64)

template <typename R, typename... Args, typename DetourT>
struct DetourCallback<R (*)(Args...), DetourT>
{
  typedef R(type)(DetourT*, Args...);
};

template <typename R, typename... Args, typename DetourT>
struct DetourCallback<R(Args...), DetourT>
{
  typedef R(type)(DetourT*, Args...);
};

#elif defined(HADESMEM_DETAIL_ARCH_X86)

HADESMEM_DETAIL_MAKE_DETOUR_CALLBACK(__cdecl)
HADESMEM_DETAIL_MAKE_DETOUR_CALLBACK(__stdcall)
HADESMEM_DETAIL_MAKE_DETOUR_CALLBACK(__fastcall)

#else
#error "[HadesMem] Unsupported architecture."
#endif

#if !defined(HADESMEM_DETAIL_NO_VECTORCALL)

HADESMEM_DETAIL_MAKE_DETOUR_CALLBACK(__vectorcall)

#endif

template <typename FuncT, typename DetourT>
using DetourCallbackT = typename DetourCallback<FuncT, DetourT>::type;

template <typename FuncT, typename DetourT> struct DetourStub;

#if defined(HADESMEM_INTEL) && defined(HADESMEM_DETAIL_ARCH_X86)

template <typename C, typename R, typename... Args, typename DetourT>
struct DetourStub<R(C::*)(Args...), DetourT>
{
  static R __fastcall Stub(C* c, void* dummy, Args... args)
  {
    (void)dummy;
    auto const detour = reinterpret_cast<DetourT*>(
      winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer);
    return detour->template Handler<R>(c, args...);
  }
};

template <typename C, typename R, typename... Args, typename DetourT>
struct DetourStub<R(C::*)(Args...) const, DetourT>
{
  static R __fastcall Stub(C const* c, void* dummy, Args... args)
  {
    (void)dummy;
    auto const detour = reinterpret_cast<DetourT*>(
      winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer);
    return detour->template Handler<R>(c, args...);
  }
};

#else // #if defined(HADESMEM_INTEL) && defined(HADESMEM_DETAIL_ARCH_X86)

template <typename C, typename R, typename... Args, typename DetourT>
struct DetourStub<R(C::*)(Args...), DetourT>
{
  static R __thiscall Stub(C* c, Args... args)
  {
    auto const detour = reinterpret_cast<DetourT*>(
      winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer);
    return detour->template Handler<R>(c, args...);
  }
};

template <typename C, typename R, typename... Args, typename DetourT>
struct DetourStub<R(C::*)(Args...) const, DetourT>
{
  static R __thiscall Stub(C const* c, Args... args)
  {
    auto const detour = reinterpret_cast<DetourT*>(
      winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer);
    return detour->template Handler<R>(c, args...);
  }
};

#endif // #if defined(HADESMEM_INTEL) && defined(HADESMEM_DETAIL_ARCH_X86)

#define HADESMEM_DETAIL_MAKE_DETOUR_STUB(call_conv)                            \
  \
template<typename R,                                                           \
         typename... Args,                                                     \
         typename DetourT> struct DetourStub<R(call_conv*)(Args...), DetourT>  \
  \
{                                                                         \
    \
static R call_conv Stub(Args... args)                                          \
    \
{                                                                       \
      auto const detour = reinterpret_cast<DetourT*>(                          \
        winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer);                \
      return detour->template Handler<R>(args...);                                      \
    \
}                                                                       \
  \
};                                                                             \
  \
template<typename R,                                                           \
         typename... Args,                                                     \
         typename DetourT> struct DetourStub<R call_conv(Args...), DetourT>    \
  \
{                                                                         \
    \
static R call_conv Stub(Args... args)                                          \
    \
{                                                                       \
      auto const detour = reinterpret_cast<DetourT*>(                          \
        winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer);                \
      return detour->template Handler<R>(args...);                                      \
    \
}                                                                       \
  \
};

#if defined(HADESMEM_DETAIL_ARCH_X64)

template <typename R, typename... Args, typename DetourT>
struct DetourStub<R (*)(Args...), DetourT>
{
  static R __stdcall Stub(Args... args)
  {
    auto const detour = reinterpret_cast<DetourT*>(
      winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer);
    return detour->template Handler<R>(args...);
  }
};

template <typename R, typename... Args, typename DetourT>
struct DetourStub<R(Args...), DetourT>
{
  static R __stdcall Stub(Args... args)
  {
    auto const detour = reinterpret_cast<DetourT*>(
      winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer);
    return detour->template Handler<R>(args...);
  }
};

#elif defined(HADESMEM_DETAIL_ARCH_X86)

HADESMEM_DETAIL_MAKE_DETOUR_STUB(__cdecl)
HADESMEM_DETAIL_MAKE_DETOUR_STUB(__stdcall)
HADESMEM_DETAIL_MAKE_DETOUR_STUB(__fastcall)

#else
#error "[HadesMem] Unsupported architecture."
#endif

#if !defined(HADESMEM_DETAIL_NO_VECTORCALL)

HADESMEM_DETAIL_MAKE_DETOUR_STUB(__vectorcall)

#endif

// WARNING! Here be dragons... GCC doesn't properly distinguish between function
// pointers with different calling conventions. Depending on the ABI version
// you're using you either get a compiler error or a linker error if you try to
// overload based on the calling convention of a function pointer taken as a
// parameter. What's even worse is that this also applies to templates! So, in
// order to work around this I'm using a disgusting hack whereby I use templates
// to detect the calling convention (if any) of the type, which the compiler can
// then use to distinguish the different overloads from each other.
/*

                     ^    ^
                    / \  //\
      |\___/|      /   \//  .\
      /O  O  \__  /    //  | \ \
     /     /  \/_/    //   |  \  \
     @___@'    \/_   //    |   \   \
        |       \/_ //     |    \    \
        |        \///      |     \     \
       _|_ /   )  //       |      \     _\
      '/,_ _ _/  ( ; -.    |    _ _\.-~        .-~~~^-.
      ,-{        _      `-.|.-~-.           .~         `.
       '/\      /                 ~-. _ .-~      .-~^-.  \
          `.   {            }                   /      \  \
        .----~-.\        \-'                 .~         \  `. \^-.
       ///.----..>    c   \             _ -~             `.  ^-`   ^-_
         ///-._ _ _ _ _ _ _}^ - - - - ~                     ~--,   .-~

*/

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

template <typename FuncT> struct IsFunction
{
  // Using FuncCallConv as a fallback because libstdc++ does not support calling
  // conventions in its std::is_function implementation.
  static bool const value =
    std::is_member_function_pointer<FuncT>::value ||
    std::is_function<FuncT>::value ||
    (std::is_pointer<FuncT>::value &&
     std::is_function<std::remove_pointer_t<FuncT>>::value) ||
    FuncCallConv<FuncT>::value != -1;
};
}
}
