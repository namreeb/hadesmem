// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <vector>
#include <type_traits>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include <boost/mpl/at.hpp>
#include <boost/preprocessor.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/parameter_types.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

namespace hadesmem
{

class Process;

class RemoteFunctionRet
{
public:
  RemoteFunctionRet(DWORD_PTR return_int_ptr, DWORD64 return_int_64, 
    float return_float, double return_double, long double return_long_double, 
    DWORD last_error);
  
  DWORD_PTR GetReturnValue() const;
  
  DWORD64 GetReturnValue64() const;
  
  float GetReturnValueFloat() const;
  
  double GetReturnValueDouble() const;
  
  long double GetReturnValueLongDouble() const;
  
  DWORD GetLastError() const;
  
private:
  DWORD_PTR int_ptr_;
  DWORD64 int_64_;
  float float_;
  double double_;
  long double long_double_;
  DWORD last_error_;
};

enum class CallConv
{
  kDefault, 
  kCdecl, 
  kStdCall, 
  kThisCall, 
  kFastCall, 
  kX64
};

// TODO: Long double support.

class CallArg
{
public:
  template <typename T>
  CallArg(T t)
    : type_(ArgType::kPtrType), 
    arg_()
  {
    static_assert(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::remove_cv<T>::type>::value || 
      std::is_same<double, typename std::remove_cv<T>::type>::value, 
      "Invalid argument type.");
    
    Initialize(t);
  }
  
  template <typename V>
  void Visit(V* v) const
  {
    switch (type_)
    {
    case ArgType::kPtrType:
      v->Visit(arg_.p);
      break;
    case ArgType::kFloatType:
      v->Visit(arg_.f);
      break;
    case ArgType::kDoubleType:
      v->Visit(arg_.d);
      break;
    default:
      BOOST_ASSERT("Invalid type." && false);
    }
  }
  
private:
  template <typename T>
  void Initialize(T t, typename std::enable_if<std::is_integral<T>::value || std::is_pointer<T>::value>::type* dummy = nullptr)
  {
    (void)dummy;
    
    type_ = ArgType::kPtrType;
    union Conv
    {
      T t;
      void* p;
    };
    Conv conv;
    conv.t = t;
    arg_.p = conv.p;
  }
  
  template <typename T>
  void Initialize(T t, typename std::enable_if<std::is_same<float, typename std::remove_cv<T>::type>::value>::type* dummy = nullptr)
  {
    (void)dummy;
    
    type_ = ArgType::kFloatType;
    arg_.f = t;
  }
  
  template <typename T>
  void Initialize(T t, typename std::enable_if<std::is_same<double, typename std::remove_cv<T>::type>::value>::type* dummy = nullptr)
  {
    (void)dummy;
    
    type_ = ArgType::kDoubleType;
    arg_.d = t;
  }
  
  enum class ArgType
  {
    kPtrType, 
    kFloatType, 
    kDoubleType
  };
  
  union Arg
  {
    void* p;
    float f;
    double d;
  };
  
  ArgType type_;
  Arg arg_;
};

RemoteFunctionRet Call(Process const& process, 
  LPCVOID address, 
  CallConv call_conv, 
  std::vector<CallArg> const& args);

std::vector<RemoteFunctionRet> CallMulti(Process const& process, 
  std::vector<LPCVOID> addresses, 
  std::vector<CallConv> call_convs, 
  std::vector<std::vector<CallArg>> const& args_full);

// TODO: Improve and clean up this mess, move to different file, etc.

template <typename FuncT>
RemoteFunctionRet Call(Process const& process, 
  LPCVOID address, 
  CallConv call_conv)
{
  static_assert(boost::function_types::function_arity<FuncT>::value == 0, "Invalid number of arguments.");
  return Call(process, address, call_conv, std::vector<CallArg>());
}

#ifndef HADESMEM_CALL_MAX_ARGS
#define HADESMEM_CALL_MAX_ARGS 10
#endif // #ifndef HADESMEM_CALL_MAX_ARGS

#if HADESMEM_CALL_MAX_ARGS > BOOST_PP_LIMIT_REPEAT
#error "[HadesMem] HADESMEM_CALL_MAX_ARGS exceeds Boost.Preprocessor repeat limit."
#endif // #if HADESMEM_CALL_MAX_ARGS > BOOST_PP_LIMIT_REPEAT

#if HADESMEM_CALL_MAX_ARGS > BOOST_PP_LIMIT_ITERATION
#error "[HadesMem] HADESMEM_CALL_MAX_ARGS exceeds Boost.Preprocessor iteration limit."
#endif // #if HADESMEM_CALL_MAX_ARGS > BOOST_PP_LIMIT_ITERATION

#define HADESMEM_CALL_ADD_ARG(z, n, unused) \
typedef typename boost::mpl::at_c<boost::function_types::parameter_types<FuncT>, n>::type A##n;\
static_assert(std::is_integral<A##n>::value || std::is_pointer<A##n>::value || std::is_floating_point<A##n>::value, "Currently only integral, pointer, or floating point types are supported.");\
static_assert(sizeof(A##n) <= sizeof(PVOID), "Currently only memsize (or smaller) types are supported.");\
static_assert(std::is_convertible<T##n, A##n>::value, "Can not convert argument to type specified in function prototype.");\
args.push_back(static_cast<A##n>(t##n));\

#define BOOST_PP_LOCAL_MACRO(n)\
template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
RemoteFunctionRet Call(Process const& process, LPCVOID address, CallConv call_conv \
  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T,&& t))\
{\
  static_assert(boost::function_types::function_arity<FuncT>::value == n, "Invalid number of arguments.");\
  std::vector<CallArg> args;\
  BOOST_PP_REPEAT(n, HADESMEM_CALL_ADD_ARG, ~)\
  return Call(process, address, call_conv, args);\
}\

#define BOOST_PP_LOCAL_LIMITS (1, HADESMEM_CALL_MAX_ARGS)

#include BOOST_PP_LOCAL_ITERATE()

#undef HADESMEM_CALL_DEFINE_ARG

#undef HADESMEM_CALL_ADD_ARG

}
