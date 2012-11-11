// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <vector>
#include <cassert>
#include <utility>
#include <type_traits>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/config.hpp>
#include <boost/mpl/at.hpp>
#include <boost/preprocessor.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/parameter_types.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

#include "hadesmem/config.hpp"
#include "hadesmem/detail/union_cast.hpp"
#include "hadesmem/detail/static_assert.hpp"

namespace hadesmem
{

class Process;

enum class CallConv
{
  kDefault, 
  kWinApi, 
  kCdecl, 
  kStdCall, 
  kThisCall, 
  kFastCall, 
  kX64
};

template <typename T>
class CallResult
{
public:
  HADESMEM_STATIC_ASSERT(std::is_integral<T>::value || 
    std::is_pointer<T>::value || 
    std::is_same<float, typename std::remove_cv<T>::type>::value || 
    std::is_same<double, typename std::remove_cv<T>::type>::value);

  CallResult(T const& result, DWORD last_error) HADESMEM_NOEXCEPT
    : result_(result), 
    last_error_(last_error)
  { }

  T GetReturnValue() const HADESMEM_NOEXCEPT
  {
    return result_;
  }

  DWORD GetLastError() const HADESMEM_NOEXCEPT
  {
    return last_error_;
  }

private:
  T result_;
  DWORD last_error_;
};

template <>
class CallResult<void>
{
public:
  CallResult(DWORD last_error) HADESMEM_NOEXCEPT
    : last_error_(last_error)
  { }

  DWORD GetLastError() const HADESMEM_NOEXCEPT
  {
    return last_error_;
  }

private:
  DWORD last_error_;
};

class CallResultRaw
{
public:
  CallResultRaw(DWORD_PTR return_int_ptr, 
    DWORD32 return_int_32, 
    DWORD64 return_int_64, 
    float return_float, 
    double return_double, 
    DWORD last_error) HADESMEM_NOEXCEPT;

  DWORD_PTR GetReturnValueIntPtr() const HADESMEM_NOEXCEPT;

  DWORD32 GetReturnValueInt32() const HADESMEM_NOEXCEPT;

  DWORD64 GetReturnValueInt64() const HADESMEM_NOEXCEPT;
  
  float GetReturnValueFloat() const HADESMEM_NOEXCEPT;
  
  double GetReturnValueDouble() const HADESMEM_NOEXCEPT;
  
  DWORD GetLastError() const HADESMEM_NOEXCEPT;
  
  template <typename T>
  T GetReturnValue() const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::remove_cv<T>::type>::value || 
      std::is_same<double, typename std::remove_cv<T>::type>::value);
    
    return GetReturnValueImpl(T());
  }
  
private:
  template <typename T>
  T GetReturnValueIntImpl(std::true_type) const HADESMEM_NOEXCEPT
  {
    return detail::UnionCast<T>(GetReturnValueInt64());
  }
  
  template <typename T>
  T GetReturnValueIntImpl(std::false_type) const HADESMEM_NOEXCEPT
  {
    return detail::UnionCast<T>(GetReturnValueInt32());
  }
  
  template <typename T>
  T GetReturnValueImpl(T /*t*/) const HADESMEM_NOEXCEPT
  {
    return GetReturnValueIntImpl<T>(std::integral_constant<bool, 
      (sizeof(T) == sizeof(DWORD64))>());
  }
  
  float GetReturnValueImpl(float /*t*/) const HADESMEM_NOEXCEPT
  {
    return GetReturnValueFloat();
  }
  
  double GetReturnValueImpl(double /*t*/) const HADESMEM_NOEXCEPT
  {
    return GetReturnValueDouble();
  }
  
  DWORD_PTR int_ptr_;
  DWORD32 int_32_;
  DWORD64 int_64_;
  float float_;
  double double_;
  DWORD last_error_;
};

class CallArg
{
public:
  template <typename T>
  explicit CallArg(T t) HADESMEM_NOEXCEPT
    : arg_(), 
    type_(ArgType::kInvalidType)
  {
    HADESMEM_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::remove_cv<T>::type>::value || 
      std::is_same<double, typename std::remove_cv<T>::type>::value);
    
    Initialize(t);
  }
  
  template <typename V>
  void Visit(V* v) const
  {
    switch (type_)
    {
    case ArgType::kInvalidType:
      assert("Invalid type." && false);
      break;
    case ArgType::kInt32Type:
      (*v)(arg_.i32);
      break;
    case ArgType::kInt64Type:
      (*v)(arg_.i64);
      break;
    case ArgType::kFloatType:
      (*v)(arg_.f);
      break;
    case ArgType::kDoubleType:
      (*v)(arg_.d);
      break;
    }
  }
  
private:
  template <typename T>
  void InitializeIntegralImpl(T t, std::false_type) HADESMEM_NOEXCEPT
  {
    type_ = ArgType::kInt32Type;
    arg_.i32 = detail::UnionCast<DWORD32>(t);
  }
  
  template <typename T>
  void InitializeIntegralImpl(T t, std::true_type) HADESMEM_NOEXCEPT
  {
    type_ = ArgType::kInt64Type;
    arg_.i64 = detail::UnionCast<DWORD64>(t);
  }
  
  template <typename T>
  void Initialize(T t) HADESMEM_NOEXCEPT
  {
    InitializeIntegralImpl(t, std::integral_constant<bool, 
      (sizeof(T) == sizeof(DWORD64))>());
  }
  
  void Initialize(float t) HADESMEM_NOEXCEPT
  {
    type_ = ArgType::kFloatType;
    arg_.f = t;
  }
  
  void Initialize(double t) HADESMEM_NOEXCEPT
  {
    type_ = ArgType::kDoubleType;
    arg_.d = t;
  }
  
  enum class ArgType
  {
    kInvalidType, 
    kInt32Type, 
    kInt64Type, 
    kFloatType, 
    kDoubleType
  };
  
  union Arg
  {
    DWORD32 i32;
    DWORD64 i64;
    float f;
    double d;
  };
  
  Arg arg_;
  ArgType type_;
};

CallResultRaw Call(Process const& process, 
  LPCVOID address, 
  CallConv call_conv, 
  std::vector<CallArg> const& args);

std::vector<CallResultRaw> CallMulti(Process const& process, 
  std::vector<LPCVOID> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full);

namespace detail
{

template <typename T>
CallResult<T> CallResultRawToCallResult(CallResultRaw const& result) 
  HADESMEM_NOEXCEPT
{
  return CallResult<T>(result.GetReturnValue<T>(), result.GetLastError());
}

template <>
inline CallResult<void> CallResultRawToCallResult(CallResultRaw const& result) 
  HADESMEM_NOEXCEPT
{
  return CallResult<void>(result.GetLastError());
}

}

#ifndef HADESMEM_NO_VARIADIC_TEMPLATES

namespace detail
{

template <typename FuncT>
struct FuncResultT
{ };

template <typename R, typename... Args>
struct FuncResultT<R (*)(Args...)>
{
  typedef R type;
};

template <typename FuncT>
struct FuncArity
{ };

template <typename R, typename... Args>
struct FuncArity<R (*)(Args...)>
{
  static int const value = sizeof...(Args);
};

template<unsigned int N, typename Head, typename... Tail>
struct GetArgN : GetArgN<N - 1, Tail...>
{ };

template<typename Head, typename... Tail>
struct GetArgN<0, Head, Tail...>
{
  // Silence a GCC warning about the lack of a virtual destructor when this 
  // type is used as a base class. This type is never actually created, so 
  // just define the destructor to shut GCC up, as there is no runtime 
  // performance penalty.
  virtual ~GetArgN() { }

  typedef Head type;
};

template <int N, typename FuncT>
struct FuncArgT
{ };

template <int N, typename R, typename... Args>
struct FuncArgT<N, R (*)(Args...)>
{
  typedef typename GetArgN<N, Args...>::type type;
};

template <typename FuncT, int N>
inline void BuildCallArgs(std::vector<CallArg>* /*call_args*/) 
  HADESMEM_NOEXCEPT
{
  return;
}

template <typename FuncT, int N, typename T, typename... Args>
void BuildCallArgs(std::vector<CallArg>* call_args, T&& arg, Args&&... args)
{
  typedef typename FuncArgT<N, FuncT>::type RealT;
  HADESMEM_STATIC_ASSERT(std::is_convertible<T, RealT>::value);
  RealT const real_arg(std::forward<T>(arg));
  call_args->emplace_back(real_arg);
  return BuildCallArgs<FuncT, N + 1>(call_args, std::forward<Args>(args)...);
}

}

template <typename FuncT, typename... Args>
CallResult<typename detail::FuncResultT<FuncT>::type> Call(
  Process const& process, LPCVOID address, CallConv call_conv, 
  Args&&... args)
{
  HADESMEM_STATIC_ASSERT(detail::FuncArity<FuncT>::value == sizeof...(args));

  std::vector<CallArg> call_args;
  call_args.reserve(sizeof...(args));
  detail::BuildCallArgs<FuncT, 0>(&call_args, args...);

  CallResultRaw const ret = Call(process, address, call_conv, call_args);
  typedef typename detail::FuncResultT<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

#else // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES

#ifndef HADESMEM_CALL_MAX_ARGS
#define HADESMEM_CALL_MAX_ARGS 20
#endif // #ifndef HADESMEM_CALL_MAX_ARGS

HADESMEM_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_REPEAT);

HADESMEM_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_ITERATION);

#define HADESMEM_CALL_ADD_ARG(z, n, unused) \
typedef typename boost::mpl::at_c<\
  boost::function_types::parameter_types<FuncT>, \
  n>::type A##n;\
HADESMEM_STATIC_ASSERT(std::is_convertible<T##n, A##n>::value);\
A##n const a##n(t##n);\
args.emplace_back(a##n);\

#define BOOST_PP_LOCAL_MACRO(n)\
template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
CallResult<typename boost::function_types::result_type<FuncT>::type> \
  Call(Process const& process, LPCVOID address, CallConv call_conv \
  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T, && t))\
{\
  HADESMEM_STATIC_ASSERT(boost::function_types::function_arity<FuncT>::value \
    == n);\
  std::vector<CallArg> args;\
  BOOST_PP_REPEAT(n, HADESMEM_CALL_ADD_ARG, ~)\
  CallResultRaw const ret = Call(process, address, call_conv, args);\
  typedef typename boost::function_types::result_type<FuncT>::type ResultT;\
return detail::CallResultRawToCallResult<ResultT>(ret);\
}\

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif // #if defined(HADESMEM_MSVC)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

#endif // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES

class MultiCall
{
public:
  explicit MultiCall(Process const* process);
  
  MultiCall(MultiCall const& other);
  
  MultiCall& operator=(MultiCall const& other);
  
  MultiCall(MultiCall&& other) HADESMEM_NOEXCEPT;
  
  MultiCall& operator=(MultiCall&& other) HADESMEM_NOEXCEPT;

#ifndef HADESMEM_NO_VARIADIC_TEMPLATES

  template <typename FuncT, typename... Args>
  void Add(LPCVOID address, CallConv call_conv, Args&&... args)
  {
    HADESMEM_STATIC_ASSERT(detail::FuncArity<FuncT>::value == sizeof...(args));

    std::vector<CallArg> call_args;
    call_args.reserve(sizeof...(args));
    detail::BuildCallArgs<FuncT, 0>(&call_args, args...);

    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(call_args);
  }

#else // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES

#define BOOST_PP_LOCAL_MACRO(n)\
template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
  void Add(LPCVOID address, CallConv call_conv \
  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T, && t))\
{\
  HADESMEM_STATIC_ASSERT(boost::function_types::function_arity<FuncT>::value \
    == n);\
  std::vector<CallArg> args;\
  BOOST_PP_REPEAT(n, HADESMEM_CALL_ADD_ARG, ~)\
  addresses_.push_back(address);\
  call_convs_.push_back(call_conv);\
  args_.push_back(args);\
}\

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif // #if defined(HADESMEM_MSVC)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

#undef HADESMEM_CALL_ADD_ARG

#endif // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES
  
  std::vector<CallResultRaw> Call() const;
  
private:
  Process const* process_;
  std::vector<LPCVOID> addresses_; 
  std::vector<CallConv> call_convs_; 
  std::vector<std::vector<CallArg>> args_;
};

}
