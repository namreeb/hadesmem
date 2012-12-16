// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <vector>
#include <cassert>
#include <utility>
#include <type_traits>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/mpl/at.hpp>
#include <boost/preprocessor.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

#include "hadesmem/config.hpp"
#include "hadesmem/detail/func_args.hpp"
#include "hadesmem/detail/func_arity.hpp"
#include "hadesmem/detail/func_result.hpp"
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

  explicit CallResult(T const& result, DWORD last_error) HADESMEM_NOEXCEPT
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
  explicit CallResult(DWORD last_error) HADESMEM_NOEXCEPT
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
  explicit CallResultRaw(DWORD_PTR return_int_ptr, 
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
  typename std::remove_cv<T>::type GetReturnValue() const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::remove_cv<T>::type>::value || 
      std::is_same<double, typename std::remove_cv<T>::type>::value);
    
    return GetReturnValueImpl(typename std::remove_cv<T>::type());
  }
  
private:
  template <typename T>
  T GetReturnValueIntImpl(std::true_type) const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::alignment_of<T>::value == 
      std::alignment_of<DWORD64>::value);
    return static_cast<T>(GetReturnValueInt64());
  }

  template <typename T>
  T GetReturnValueIntImpl(std::false_type) const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::alignment_of<T>::value == 
      std::alignment_of<DWORD32>::value);
    return static_cast<T>(GetReturnValueInt32());
  }

  template <typename T>
  T* GetReturnValuePtrImpl(std::true_type) const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::alignment_of<T*>::value == 
      std::alignment_of<DWORD64>::value);
    return reinterpret_cast<T*>(GetReturnValueInt64());
  }

  template <typename T>
  T* GetReturnValuePtrImpl(std::false_type) const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::alignment_of<T*>::value == 
      std::alignment_of<DWORD32>::value);
    return reinterpret_cast<T*>(GetReturnValueInt32());
  }

  template <typename T>
  T GetReturnValueImpl(T /*t*/) const HADESMEM_NOEXCEPT
  {
    return GetReturnValueIntImpl<T>(std::integral_constant<bool, 
      (sizeof(T) == sizeof(DWORD64))>());
  }

  template <typename T>
  T* GetReturnValueImpl(T* /*t*/) const HADESMEM_NOEXCEPT
  {
    return GetReturnValuePtrImpl<T>(std::integral_constant<bool, 
      (sizeof(void*) == sizeof(DWORD64))>());
  }

  template <typename T>
  T const* GetReturnValueImpl(T const* /*t*/) const HADESMEM_NOEXCEPT
  {
    return GetReturnValuePtrImpl<T>(std::integral_constant<bool, 
      (sizeof(void*) == sizeof(DWORD64))>());
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
    arg_.i32 = static_cast<DWORD32>(t);
  }
  
  template <typename T>
  void InitializeIntegralImpl(T t, std::true_type) HADESMEM_NOEXCEPT
  {
    type_ = ArgType::kInt64Type;
    arg_.i64 = static_cast<DWORD64>(t);
  }

  template <typename T>
  void InitializePointerImpl(T const* t, std::false_type) HADESMEM_NOEXCEPT
  {
    type_ = ArgType::kInt32Type;
    arg_.i32 = reinterpret_cast<DWORD32>(t);
  }

  template <typename T>
  void InitializePointerImpl(T const* t, std::true_type) HADESMEM_NOEXCEPT
  {
    type_ = ArgType::kInt64Type;
    arg_.i64 = reinterpret_cast<DWORD64>(t);
  }
  
  template <typename T>
  void Initialize(T t) HADESMEM_NOEXCEPT
  {
    InitializeIntegralImpl(t, std::integral_constant<bool, 
      (sizeof(T) == sizeof(DWORD64))>());
  }

  template <typename T>
  void Initialize(T const* t) HADESMEM_NOEXCEPT
  {
    InitializePointerImpl(t, std::integral_constant<bool, 
      (sizeof(void*) == sizeof(DWORD64))>());
  }

  template <typename T>
  void Initialize(T* t) HADESMEM_NOEXCEPT
  {
    Initialize(static_cast<T const*>(t));
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

typedef void (*FnPtr)();
HADESMEM_STATIC_ASSERT(sizeof(FnPtr) == sizeof(void*));

CallResultRaw Call(Process const& process, 
  FnPtr address, 
  CallConv call_conv, 
  std::vector<CallArg> const& args);

std::vector<CallResultRaw> CallMulti(Process const& process, 
  std::vector<FnPtr> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full);

namespace detail
{

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif // #if defined(HADESMEM_MSVC)

template <typename FuncT, int N, typename T>
void AddCallArg(std::vector<CallArg>* call_args, T&& arg)
{
  typedef typename detail::FuncArgs<FuncT>::type FuncArgs;
  typedef typename boost::mpl::at_c<FuncArgs, N>::type RealT;
  HADESMEM_STATIC_ASSERT(std::is_convertible<T, RealT>::value);
  RealT const real_arg(std::forward<T>(arg));
  call_args->emplace_back(std::move(real_arg));
}

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

}

#ifndef HADESMEM_NO_VARIADIC_TEMPLATES

namespace detail
{

template <typename FuncT, int N>
void BuildCallArgs(std::vector<CallArg>* /*call_args*/) HADESMEM_NOEXCEPT
{
  return;
}

template <typename FuncT, int N, typename T, typename... Args>
void BuildCallArgs(std::vector<CallArg>* call_args, T&& arg, 
  Args&&... args)
{
  AddCallArg<FuncT, N>(call_args, std::forward<T>(arg));
  return BuildCallArgs<FuncT, N + 1>(call_args, std::forward<Args>(args)...);
}

}

// TODO: Detect calling convention from function pointer type instead of 
// passing it explicitly.
// TODO: Support more 'complex' function pointer types (incl cv-qualified 
// member functions, explicit call convs, noexcept specs, etc).
template <typename FuncT, typename... Args>
CallResult<typename detail::FuncResult<FuncT>::type> Call(
  Process const& process, FnPtr address, CallConv call_conv, 
  Args&&... args)
{
  HADESMEM_STATIC_ASSERT(detail::FuncArity<FuncT>::value == sizeof...(args));

  std::vector<CallArg> call_args;
  call_args.reserve(sizeof...(args));
  detail::BuildCallArgs<FuncT, 0>(&call_args, args...);

  CallResultRaw const ret = Call(process, address, call_conv, call_args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

#else // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES

HADESMEM_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_REPEAT);

HADESMEM_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < BOOST_PP_LIMIT_ITERATION);

#define HADESMEM_CHECK_FUNC_ARITY(n) \
  HADESMEM_STATIC_ASSERT(detail::FuncArity<FuncT>::value == n)

#define HADESMEM_CALL_ADD_ARG(n) \
  detail::AddCallArg<FuncT, n>(&args, std::forward<T##n>(t##n))

#define HADESMEM_CALL_ADD_ARG_WRAPPER(z, n, unused) \
  HADESMEM_CALL_ADD_ARG(n);

#define BOOST_PP_LOCAL_MACRO(n) \
template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
CallResult<typename detail::FuncResult<FuncT>::type>\
  Call(Process const& process, FnPtr address, CallConv call_conv \
  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T, && t))\
{\
  HADESMEM_CHECK_FUNC_ARITY(n);\
  std::vector<CallArg> args;\
  BOOST_PP_REPEAT(n, HADESMEM_CALL_ADD_ARG_WRAPPER, ~)\
  CallResultRaw const ret = Call(process, address, call_conv, args);\
  typedef typename detail::FuncResult<FuncT>::type ResultT;\
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

  // TODO: Detect calling convention from function pointer type.
  // TODO: Support more 'complex' function pointer types (incl cv-qualified 
  // member functions, explicit call convs, noexcept specs, etc).
  template <typename FuncT, typename... Args>
  void Add(FnPtr address, CallConv call_conv, Args&&... args)
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

#define BOOST_PP_LOCAL_MACRO(n) \
  template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
  void Add(FnPtr address, CallConv call_conv \
    BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T, && t))\
  {\
    HADESMEM_CHECK_FUNC_ARITY(n);\
    std::vector<CallArg> args;\
    BOOST_PP_REPEAT(n, HADESMEM_CALL_ADD_ARG_WRAPPER, ~)\
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

#undef HADESMEM_CHECK_FUNC_ARITY

#undef HADESMEM_CALL_ADD_ARG

#undef HADESMEM_CALL_ADD_ARG_WRAPPER

#endif // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES
  
  std::vector<CallResultRaw> Call() const;
  
private:
  Process const* process_;
  std::vector<FnPtr> addresses_; 
  std::vector<CallConv> call_convs_; 
  std::vector<std::vector<CallArg>> args_;
};

}
