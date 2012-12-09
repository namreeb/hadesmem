// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <vector>
#include <cassert>
#include <utility>
#include <type_traits>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/mpl/at.hpp>
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
  typedef typename boost::mpl::at<FuncArgs, boost::mpl::int_<N>>::type RealT;
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

#define HADESMEM_CALL_ADD_ARG(n) \
  detail::AddCallArg<FuncT, n>(&args, std::forward<T##n>(t##n))

#define HADESMEM_CHECK_FUNC_ARITY(n) \
  HADESMEM_STATIC_ASSERT(detail::FuncArity<FuncT>::value == n)

template <typename FuncT>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv)
{
  HADESMEM_CHECK_FUNC_ARITY(0);
  std::vector<CallArg> args;
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

template <typename FuncT, typename T0>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv, 
  T0&& t0)
{
  HADESMEM_CHECK_FUNC_ARITY(1);
  std::vector<CallArg> args;
  HADESMEM_CALL_ADD_ARG(0);
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

template <typename FuncT, typename T0, typename T1>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv, 
  T0&& t0, T1&& t1)
{
  HADESMEM_CHECK_FUNC_ARITY(2);
  std::vector<CallArg> args;
  HADESMEM_CALL_ADD_ARG(0);
  HADESMEM_CALL_ADD_ARG(1);
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

template <typename FuncT, typename T0, typename T1, typename T2>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv, 
  T0&& t0, T1&& t1, T2&& t2)
{
  HADESMEM_CHECK_FUNC_ARITY(3);
  std::vector<CallArg> args;
  HADESMEM_CALL_ADD_ARG(0);
  HADESMEM_CALL_ADD_ARG(1);
  HADESMEM_CALL_ADD_ARG(2);
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

template <typename FuncT, typename T0, typename T1, typename T2, typename T3>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv, 
  T0&& t0, T1&& t1, T2&& t2, T3&& t3)
{
  HADESMEM_CHECK_FUNC_ARITY(4);
  std::vector<CallArg> args;
  HADESMEM_CALL_ADD_ARG(0);
  HADESMEM_CALL_ADD_ARG(1);
  HADESMEM_CALL_ADD_ARG(2);
  HADESMEM_CALL_ADD_ARG(3);
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
  typename T4>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv, 
  T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4)
{
  HADESMEM_CHECK_FUNC_ARITY(5);
  std::vector<CallArg> args;
  HADESMEM_CALL_ADD_ARG(0);
  HADESMEM_CALL_ADD_ARG(1);
  HADESMEM_CALL_ADD_ARG(2);
  HADESMEM_CALL_ADD_ARG(3);
  HADESMEM_CALL_ADD_ARG(4);
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
  typename T4, typename T5>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv, 
  T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5)
{
  HADESMEM_CHECK_FUNC_ARITY(6);
  std::vector<CallArg> args;
  HADESMEM_CALL_ADD_ARG(0);
  HADESMEM_CALL_ADD_ARG(1);
  HADESMEM_CALL_ADD_ARG(2);
  HADESMEM_CALL_ADD_ARG(3);
  HADESMEM_CALL_ADD_ARG(4);
  HADESMEM_CALL_ADD_ARG(5);
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
  typename T4, typename T5, typename T6>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv, 
  T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6)
{
  HADESMEM_CHECK_FUNC_ARITY(7);
  std::vector<CallArg> args;
  HADESMEM_CALL_ADD_ARG(0);
  HADESMEM_CALL_ADD_ARG(1);
  HADESMEM_CALL_ADD_ARG(2);
  HADESMEM_CALL_ADD_ARG(3);
  HADESMEM_CALL_ADD_ARG(4);
  HADESMEM_CALL_ADD_ARG(5);
  HADESMEM_CALL_ADD_ARG(6);
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
  typename T4, typename T5, typename T6, typename T7>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv, 
  T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7)
{
  HADESMEM_CHECK_FUNC_ARITY(8);
  std::vector<CallArg> args;
  HADESMEM_CALL_ADD_ARG(0);
  HADESMEM_CALL_ADD_ARG(1);
  HADESMEM_CALL_ADD_ARG(2);
  HADESMEM_CALL_ADD_ARG(3);
  HADESMEM_CALL_ADD_ARG(4);
  HADESMEM_CALL_ADD_ARG(5);
  HADESMEM_CALL_ADD_ARG(6);
  HADESMEM_CALL_ADD_ARG(7);
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
  typename T4, typename T5, typename T6, typename T7, typename T8>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv, 
  T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, 
  T8&& t8)
{
  HADESMEM_CHECK_FUNC_ARITY(9);
  std::vector<CallArg> args;
  HADESMEM_CALL_ADD_ARG(0);
  HADESMEM_CALL_ADD_ARG(1);
  HADESMEM_CALL_ADD_ARG(2);
  HADESMEM_CALL_ADD_ARG(3);
  HADESMEM_CALL_ADD_ARG(4);
  HADESMEM_CALL_ADD_ARG(5);
  HADESMEM_CALL_ADD_ARG(6);
  HADESMEM_CALL_ADD_ARG(7);
  HADESMEM_CALL_ADD_ARG(8);
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
  typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
CallResult<typename detail::FuncResult<FuncT>::type> 
  Call(Process const& process, FnPtr address, CallConv call_conv, 
  T0&& t0, T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, 
  T8&& t8, T9&& t9)
{
  HADESMEM_CHECK_FUNC_ARITY(10);
  std::vector<CallArg> args;
  HADESMEM_CALL_ADD_ARG(0);
  HADESMEM_CALL_ADD_ARG(1);
  HADESMEM_CALL_ADD_ARG(2);
  HADESMEM_CALL_ADD_ARG(3);
  HADESMEM_CALL_ADD_ARG(4);
  HADESMEM_CALL_ADD_ARG(5);
  HADESMEM_CALL_ADD_ARG(6);
  HADESMEM_CALL_ADD_ARG(7);
  HADESMEM_CALL_ADD_ARG(8);
  HADESMEM_CALL_ADD_ARG(9);
  CallResultRaw const ret = Call(process, address, call_conv, args);
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

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

  template <typename FuncT>
  void Add(FnPtr address, CallConv call_conv)
  {
    HADESMEM_CHECK_FUNC_ARITY(0);
    std::vector<CallArg> args;
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

  template <typename FuncT, typename T0>
  void Add(FnPtr address, CallConv call_conv, T0&& t0)
  {
    HADESMEM_CHECK_FUNC_ARITY(1);
    std::vector<CallArg> args;
    HADESMEM_CALL_ADD_ARG(0);
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

  template <typename FuncT, typename T0, typename T1>
  void Add(FnPtr address, CallConv call_conv, T0&& t0, T1&& t1)
  {
    HADESMEM_CHECK_FUNC_ARITY(2);
    std::vector<CallArg> args;
    HADESMEM_CALL_ADD_ARG(0);
    HADESMEM_CALL_ADD_ARG(1);
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

  template <typename FuncT, typename T0, typename T1, typename T2>
  void Add(FnPtr address, CallConv call_conv, T0&& t0, T1&& t1, T2&& t2)
  {
    HADESMEM_CHECK_FUNC_ARITY(3);
    std::vector<CallArg> args;
    HADESMEM_CALL_ADD_ARG(0);
    HADESMEM_CALL_ADD_ARG(1);
    HADESMEM_CALL_ADD_ARG(2);
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

  template <typename FuncT, typename T0, typename T1, typename T2, typename T3>
  void Add(FnPtr address, CallConv call_conv, T0&& t0, T1&& t1, T2&& t2, 
    T3&& t3)
  {
    HADESMEM_CHECK_FUNC_ARITY(4);
    std::vector<CallArg> args;
    HADESMEM_CALL_ADD_ARG(0);
    HADESMEM_CALL_ADD_ARG(1);
    HADESMEM_CALL_ADD_ARG(2);
    HADESMEM_CALL_ADD_ARG(3);
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

  template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
    typename T4>
  void Add(FnPtr address, CallConv call_conv, T0&& t0, T1&& t1, T2&& t2, 
    T3&& t3, T4&& t4)
  {
    HADESMEM_CHECK_FUNC_ARITY(5);
    std::vector<CallArg> args;
    HADESMEM_CALL_ADD_ARG(0);
    HADESMEM_CALL_ADD_ARG(1);
    HADESMEM_CALL_ADD_ARG(2);
    HADESMEM_CALL_ADD_ARG(3);
    HADESMEM_CALL_ADD_ARG(4);
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

  template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
    typename T4, typename T5>
  void Add(FnPtr address, CallConv call_conv, T0&& t0, T1&& t1, T2&& t2, 
    T3&& t3, T4&& t4, T5&& t5)
  {
    HADESMEM_CHECK_FUNC_ARITY(6);
    std::vector<CallArg> args;
    HADESMEM_CALL_ADD_ARG(0);
    HADESMEM_CALL_ADD_ARG(1);
    HADESMEM_CALL_ADD_ARG(2);
    HADESMEM_CALL_ADD_ARG(3);
    HADESMEM_CALL_ADD_ARG(4);
    HADESMEM_CALL_ADD_ARG(5);
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

  template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
    typename T4, typename T5, typename T6>
  void Add(FnPtr address, CallConv call_conv, T0&& t0, T1&& t1, T2&& t2, 
    T3&& t3, T4&& t4, T5&& t5, T6&& t6)
  {
    HADESMEM_CHECK_FUNC_ARITY(7);
    std::vector<CallArg> args;
    HADESMEM_CALL_ADD_ARG(0);
    HADESMEM_CALL_ADD_ARG(1);
    HADESMEM_CALL_ADD_ARG(2);
    HADESMEM_CALL_ADD_ARG(3);
    HADESMEM_CALL_ADD_ARG(4);
    HADESMEM_CALL_ADD_ARG(5);
    HADESMEM_CALL_ADD_ARG(6);
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

  template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
    typename T4, typename T5, typename T6, typename T7>
  void Add(FnPtr address, CallConv call_conv, T0&& t0, T1&& t1, T2&& t2, 
    T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7)
  {
    HADESMEM_CHECK_FUNC_ARITY(8);
    std::vector<CallArg> args;
    HADESMEM_CALL_ADD_ARG(0);
    HADESMEM_CALL_ADD_ARG(1);
    HADESMEM_CALL_ADD_ARG(2);
    HADESMEM_CALL_ADD_ARG(3);
    HADESMEM_CALL_ADD_ARG(4);
    HADESMEM_CALL_ADD_ARG(5);
    HADESMEM_CALL_ADD_ARG(6);
    HADESMEM_CALL_ADD_ARG(7);
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

  template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
    typename T4, typename T5, typename T6, typename T7, typename T8>
  void Add(FnPtr address, CallConv call_conv, T0&& t0, T1&& t1, T2&& t2, 
    T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, T8&& t8)
  {
    HADESMEM_CHECK_FUNC_ARITY(9);
    std::vector<CallArg> args;
    HADESMEM_CALL_ADD_ARG(0);
    HADESMEM_CALL_ADD_ARG(1);
    HADESMEM_CALL_ADD_ARG(2);
    HADESMEM_CALL_ADD_ARG(3);
    HADESMEM_CALL_ADD_ARG(4);
    HADESMEM_CALL_ADD_ARG(5);
    HADESMEM_CALL_ADD_ARG(6);
    HADESMEM_CALL_ADD_ARG(7);
    HADESMEM_CALL_ADD_ARG(8);
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

  template <typename FuncT, typename T0, typename T1, typename T2, typename T3, 
    typename T4, typename T5, typename T6, typename T7, typename T8, 
    typename T9>
  void Add(FnPtr address, CallConv call_conv, T0&& t0, T1&& t1, T2&& t2, 
    T3&& t3, T4&& t4, T5&& t5, T6&& t6, T7&& t7, T8&& t8, T9&& t9)
  {
    HADESMEM_CHECK_FUNC_ARITY(10);
    std::vector<CallArg> args;
    HADESMEM_CALL_ADD_ARG(0);
    HADESMEM_CALL_ADD_ARG(1);
    HADESMEM_CALL_ADD_ARG(2);
    HADESMEM_CALL_ADD_ARG(3);
    HADESMEM_CALL_ADD_ARG(4);
    HADESMEM_CALL_ADD_ARG(5);
    HADESMEM_CALL_ADD_ARG(6);
    HADESMEM_CALL_ADD_ARG(7);
    HADESMEM_CALL_ADD_ARG(8);
    HADESMEM_CALL_ADD_ARG(9);
    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(args);
  }

#undef HADESMEM_CALL_ADD_ARG

#undef HADESMEM_CHECK_FUNC_ARITY

#endif // #ifndef HADESMEM_NO_VARIADIC_TEMPLATES
  
  std::vector<CallResultRaw> Call() const;
  
private:
  Process const* process_;
  std::vector<FnPtr> addresses_; 
  std::vector<CallConv> call_convs_; 
  std::vector<std::vector<CallArg>> args_;
};

}
