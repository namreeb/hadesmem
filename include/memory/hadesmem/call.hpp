// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <vector>
#include <utility>
#include <type_traits>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/mpl/at.hpp>
#include <boost/preprocessor.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/parameter_types.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

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

class CallResult
{
public:
  CallResult(DWORD_PTR return_int_ptr, 
    DWORD32 return_int_32, 
    DWORD64 return_int_64, 
    float return_float, 
    double return_double, 
    DWORD last_error) BOOST_NOEXCEPT;

  CallResult(CallResult const& other) BOOST_NOEXCEPT;

  CallResult& operator=(CallResult const& other) BOOST_NOEXCEPT;

  CallResult(CallResult&& other) BOOST_NOEXCEPT;

  CallResult& operator=(CallResult&& other) BOOST_NOEXCEPT;

  ~CallResult();
  
  DWORD_PTR GetReturnValueIntPtr() const BOOST_NOEXCEPT;

  DWORD32 GetReturnValueInt32() const BOOST_NOEXCEPT;

  DWORD64 GetReturnValueInt64() const BOOST_NOEXCEPT;
  
  float GetReturnValueFloat() const BOOST_NOEXCEPT;
  
  double GetReturnValueDouble() const BOOST_NOEXCEPT;
  
  DWORD GetLastError() const BOOST_NOEXCEPT;
  
  template <typename T>
  T GetReturnValue() const BOOST_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::remove_cv<T>::type>::value || 
      std::is_same<double, typename std::remove_cv<T>::type>::value);
    
    return GetReturnValueImpl(T());
  }
  
private:
  template <typename T>
  T GetReturnValueIntImpl(std::true_type const&) const BOOST_NOEXCEPT
  {
    union Conv
    {
      T t;
      DWORD64 i;
    };
    Conv conv;
    conv.i = GetReturnValueInt64();
    return conv.t;
  }
  
  template <typename T>
  T GetReturnValueIntImpl(std::false_type const&) const BOOST_NOEXCEPT
  {
    union Conv
    {
      T t;
      DWORD32 i;
    };
    Conv conv;
    conv.i = GetReturnValueInt32();
    return conv.t;
  }
  
  template <typename T>
  T GetReturnValueImpl(T /*t*/) const BOOST_NOEXCEPT
  {
    return GetReturnValueIntImpl<T>(std::integral_constant<bool, 
      (sizeof(T) == sizeof(DWORD64))>());
  }
  
  float GetReturnValueImpl(float /*t*/) const BOOST_NOEXCEPT
  {
    return GetReturnValueFloat();
  }
  
  double GetReturnValueImpl(double /*t*/) const BOOST_NOEXCEPT
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
  CallArg(T t) BOOST_NOEXCEPT
    : type_(ArgType::kInvalidType), 
    arg_()
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
      BOOST_ASSERT("Invalid type." && false);
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
    default:
      BOOST_ASSERT("Invalid type." && false);
      break;
    }
  }
  
private:
  template <typename T>
  void InitializeIntegralImpl(T t, std::false_type const&) BOOST_NOEXCEPT
  {
    type_ = ArgType::kInt32Type;
    union Conv
    {
      T t;
      DWORD32 i;
    };
    Conv conv;
    conv.t = t;
    arg_.i32 = conv.i;
  }
  
  template <typename T>
  void InitializeIntegralImpl(T t, std::true_type const&) BOOST_NOEXCEPT
  {
    type_ = ArgType::kInt64Type;
    union Conv
    {
      T t;
      DWORD64 i;
    };
    Conv conv;
    conv.t = t;
    arg_.i64 = conv.i;
  }
  
  template <typename T>
  void Initialize(T t) BOOST_NOEXCEPT
  {
    InitializeIntegralImpl(t, std::integral_constant<bool, 
      (sizeof(T) == sizeof(DWORD64))>());
  }
  
  void Initialize(float t) BOOST_NOEXCEPT
  {
    type_ = ArgType::kFloatType;
    arg_.f = t;
  }
  
  void Initialize(double t) BOOST_NOEXCEPT
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
  
  ArgType type_;
  Arg arg_;
};

CallResult Call(Process const& process, 
  LPCVOID address, 
  CallConv call_conv, 
  std::vector<CallArg> const& args);

std::vector<CallResult> CallMulti(Process const& process, 
  std::vector<LPCVOID> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full);

template <typename T>
struct VoidToInt
{
  typedef T type;
};

template <>
struct VoidToInt<void>
{
  typedef int type;
};

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
A##n a##n = t##n;\
args.push_back(a##n);\

#define BOOST_PP_LOCAL_MACRO(n)\
template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
std::pair<typename VoidToInt<\
  typename boost::function_types::result_type<FuncT>::type>::type, DWORD> \
  Call(Process const& process, LPCVOID address, CallConv call_conv \
  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T, t))\
{\
  HADESMEM_STATIC_ASSERT(boost::function_types::function_arity<FuncT>::value \
    == n);\
  std::vector<CallArg> args;\
  BOOST_PP_REPEAT(n, HADESMEM_CALL_ADD_ARG, ~)\
  CallResult const ret = Call(process, address, call_conv, args);\
  typedef typename VoidToInt<\
    typename boost::function_types::result_type<FuncT>::type>::type ResultT;\
  return std::make_pair(ret.GetReturnValue<ResultT>(), ret.GetLastError());\
}\

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif // #if defined(HADESMEM_GCC)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

#undef HADESMEM_CALL_DEFINE_ARG

#undef HADESMEM_CALL_ADD_ARG

class MultiCall
{
public:
  explicit MultiCall(Process const* process);
  
  MultiCall(MultiCall const& other);
  
  MultiCall& operator=(MultiCall const& other);
  
  MultiCall(MultiCall&& other) BOOST_NOEXCEPT;
  
  MultiCall& operator=(MultiCall&& other) BOOST_NOEXCEPT;
  
  ~MultiCall();
  
#define HADESMEM_CALL_ADD_ARG(z, n, unused) \
typedef typename boost::mpl::at_c<\
  boost::function_types::parameter_types<FuncT>, \
  n>::type A##n;\
HADESMEM_STATIC_ASSERT(std::is_convertible<T##n, A##n>::value);\
A##n a##n = t##n;\
args.push_back(a##n);\

#define BOOST_PP_LOCAL_MACRO(n)\
template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
  void Add(Process const& process, LPCVOID address, CallConv call_conv \
  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T, t))\
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

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif // #if defined(HADESMEM_GCC)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

#undef HADESMEM_CALL_DEFINE_ARG

#undef HADESMEM_CALL_ADD_ARG
  
  std::vector<CallResult> Call() const;
  
private:
  Process const* process_;
  std::vector<LPCVOID> addresses_; 
  std::vector<CallConv> call_convs_; 
  std::vector<std::vector<CallArg>> args_;
};

}
