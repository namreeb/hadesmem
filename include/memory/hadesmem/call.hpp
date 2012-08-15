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
      std::is_same<double, typename std::remove_cv<T>::type>::value || 
      (std::is_integral<T>::value && sizeof(T) <= sizeof(DWORD64)), 
      "Currently only memsize (or smaller) types are supported (doubles "
      "and 64-bit integrals excepted).");
    
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
    case ArgType::kInt64Type:
      (*v)(arg_.i);
      break;
    default:
      BOOST_ASSERT("Invalid type." && false);
    }
  }
  
private:
  template <typename T>
  void InitializeIntegralImpl(T t, typename std::enable_if<sizeof(T) <= 
    sizeof(DWORD_PTR)>::type* /*dummy*/ = 0)
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
  void InitializeIntegralImpl(T t, typename std::enable_if<sizeof(void*) != 
    sizeof(DWORD64) && sizeof(T) == sizeof(DWORD64)>::type* /*dummy*/ = 0)
  {
    type_ = ArgType::kInt64Type;
    union Conv
    {
      T t;
      DWORD64 i;
    };
    Conv conv;
    conv.t = t;
    arg_.i = conv.i;
  }
  
  template <typename T>
  void Initialize(T t, typename std::enable_if<std::is_integral<T>::value || 
    std::is_pointer<T>::value>::type* /*dummy*/ = nullptr)
  {
    InitializeIntegralImpl(t);
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
    kDoubleType, 
    kInt64Type
  };
  
  union Arg
  {
    void* p;
    float f;
    double d;
    DWORD64 i;
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
  explicit MultiCall(Process const* process)
    : process_(process), 
    addresses_(), 
    call_convs_(), 
    args_()
  { }
  
  MultiCall(MultiCall const& other)
    : process_(other.process_), 
    addresses_(other.addresses_), 
    call_convs_(other.call_convs_), 
    args_(other.args_)
  { }
  
  MultiCall& operator=(MultiCall const& other)
  {
    process_ = other.process_;
    addresses_ = other.addresses_;
    call_convs_ = other.call_convs_;
    args_ = other.args_;
    
    return *this;
  }
  
  MultiCall(MultiCall&& other) BOOST_NOEXCEPT
    : process_(other.process_), 
    addresses_(std::move(other.addresses_)), 
    call_convs_(std::move(other.call_convs_)), 
    args_(std::move(other.args_))
  {
    other.process_ = nullptr;
  }
  
  MultiCall& operator=(MultiCall&& other) BOOST_NOEXCEPT
  {
    process_ = other.process_;
    other.process_ = nullptr;
    
    addresses_ = std::move(other.addresses_);
    
    call_convs_ = std::move(other.call_convs_);
    
    args_ = std::move(other.args_);
    
    return *this;
  }
  
  ~MultiCall()
  { }
  
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
  void Add(Process const& process, LPCVOID address, CallConv call_conv \
  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T, t))\
{\
  static_assert(boost::function_types::function_arity<FuncT>::value == n, \
    "Invalid number of arguments.");\
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
  
  std::vector<RemoteFunctionRet> Call()
  {
    return hadesmem::CallMulti(*process_, addresses_, call_convs_, args_);
  }
  
private:
  Process const* process_;
  std::vector<LPCVOID> addresses_; 
  std::vector<CallConv> call_convs_; 
  std::vector<std::vector<CallArg>> args_;
};

}
