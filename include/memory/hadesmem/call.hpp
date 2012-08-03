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
#include <boost/mpl/at.hpp>
#include <boost/preprocessor.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/parameter_types.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

// TODO: BOOST_NOEXCEPT annotations.

// TODO: Improve and clean up this mess, move details to different files, etc.

namespace hadesmem
{

class Process;

class RemoteFunctionRet
{
public:
  RemoteFunctionRet(DWORD_PTR return_int_ptr, DWORD64 return_int_64, 
    float return_float, double return_double, DWORD last_error);
  
  DWORD_PTR GetReturnValue() const;
  
  DWORD64 GetReturnValue64() const;
  
  float GetReturnValueFloat() const;
  
  double GetReturnValueDouble() const;
  
  DWORD GetLastError() const;
  
  template <typename T>
  T GetReturnValue() const
  {
    static_assert(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::remove_cv<T>::type>::value || 
      std::is_same<double, typename std::remove_cv<T>::type>::value, 
      "Only integral, pointer, or floating point types are supported.");
    
    return GetReturnValueImpl<T>();
  }
  
private:
  template <typename T>
  T GetReturnValueIntImpl(typename std::enable_if<sizeof(T) == 
    sizeof(DWORD64)>::type* /*dummy*/ = nullptr) const
  {
    union Conv
    {
      T t;
      DWORD64 d;
    };
    Conv conv;
    conv.d = GetReturnValue64();
    return conv.t;
  }
  
  template <typename T>
  T GetReturnValueIntImpl(typename std::enable_if<sizeof(T) != 
    sizeof(DWORD64)>::type* /*dummy*/ = nullptr) const
  {
    union Conv
    {
      T t;
      DWORD_PTR d;
    };
    Conv conv;
    conv.d = GetReturnValue();
    return conv.t;
  }
  
  template <typename T>
  T GetReturnValueImpl(typename std::enable_if<std::is_integral<T>::value || 
    std::is_pointer<T>::value>::type* /*dummy*/ = nullptr) const
  {
    return GetReturnValueIntImpl<T>();
  }
  
  template <typename T>
  T GetReturnValueImpl(typename std::enable_if<std::is_same<float, 
    typename std::remove_cv<T>::type>::value>::type* /*dummy*/ = nullptr) const
  {
    return GetReturnValueFloat();
  }
  
  template <typename T>
  T GetReturnValueImpl(typename std::enable_if<std::is_same<double, 
    typename std::remove_cv<T>::type>::value>::type* /*dummy*/ = nullptr) const
  {
    return GetReturnValueDouble();
  }
  
  DWORD_PTR int_ptr_;
  DWORD64 int_64_;
  float float_;
  double double_;
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
      "Only integral, pointer, or floating point types are supported.");
    
    static_assert(sizeof(T) <= sizeof(void*) || 
      std::is_same<double, typename std::remove_cv<T>::type>::value, 
      "Currently only memsize (or smaller) types are supported (doubles "
      "excepted).");
    
    Initialize(t);
  }
  
  template <typename V>
  void Visit(V* v) const
  {
    switch (type_)
    {
    case ArgType::kPtrType:
      (*v)(arg_.p);
      break;
    case ArgType::kFloatType:
      (*v)(arg_.f);
      break;
    case ArgType::kDoubleType:
      (*v)(arg_.d);
      break;
    default:
      BOOST_ASSERT("Invalid type." && false);
    }
  }
  
private:
  template <typename T>
  void Initialize(T t, typename std::enable_if<std::is_integral<T>::value || 
    std::is_pointer<T>::value>::type* /*dummy*/ = nullptr)
  {
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
  void Initialize(T t, typename std::enable_if<std::is_same<float, 
    typename std::remove_cv<T>::type>::value>::type* /*dummy*/ = nullptr)
  {
    type_ = ArgType::kFloatType;
    arg_.f = t;
  }
  
  template <typename T>
  void Initialize(T t, typename std::enable_if<std::is_same<double, 
    typename std::remove_cv<T>::type>::value>::type* /*dummy*/ = nullptr)
  {
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
  std::vector<LPCVOID> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full);

#ifndef HADESMEM_CALL_MAX_ARGS
#define HADESMEM_CALL_MAX_ARGS 20
#endif // #ifndef HADESMEM_CALL_MAX_ARGS

static_assert(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_REPEAT, 
  "HADESMEM_CALL_MAX_ARGS exceeds Boost.Preprocessor repeat limit.");

static_assert(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_ITERATION, 
  "HADESMEM_CALL_MAX_ARGS exceeds Boost.Preprocessor iteration limit.");

#define HADESMEM_CALL_ADD_ARG(z, n, unused) \
typedef typename boost::mpl::at_c<\
  boost::function_types::parameter_types<FuncT>, \
  n>::type A##n;\
static_assert(std::is_convertible<T##n, A##n>::value, \
  "Can not convert argument to type specified in function prototype.");\
A##n a##n = t##n;\
args.push_back(a##n);\

#define BOOST_PP_LOCAL_MACRO(n)\
template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
std::pair<typename boost::function_types::result_type<FuncT>::type, DWORD> \
  Call(Process const& process, LPCVOID address, CallConv call_conv \
  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T, t))\
{\
  static_assert(boost::function_types::function_arity<FuncT>::value == n, \
    "Invalid number of arguments.");\
  std::vector<CallArg> args;\
  BOOST_PP_REPEAT(n, HADESMEM_CALL_ADD_ARG, ~)\
  RemoteFunctionRet const ret = Call(process, address, call_conv, args);\
  typedef typename boost::function_types::result_type<FuncT>::type ResultT;\
  return std::make_pair(ret.GetReturnValue<ResultT>(), ret.GetLastError());\
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

#undef HADESMEM_CALL_DEFINE_ARG

#undef HADESMEM_CALL_ADD_ARG

}
