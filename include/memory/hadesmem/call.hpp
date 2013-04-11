// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <vector>
#include <utility>
#include <type_traits>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/assert.hpp>
#include <boost/mpl/at.hpp>
#include <boost/variant.hpp>
#include <boost/preprocessor.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/detail/func_args.hpp>
#include <hadesmem/detail/func_arity.hpp>
#include <hadesmem/detail/func_result.hpp>
#include <hadesmem/detail/static_assert.hpp>

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
  explicit CallResultRaw(DWORD64 return_int_64, float return_float, 
    double return_double, DWORD last_error);

  CallResultRaw(CallResultRaw const& other);

  CallResultRaw& operator=(CallResultRaw const& other);

  CallResultRaw(CallResultRaw&& other) HADESMEM_NOEXCEPT;

  CallResultRaw& operator=(CallResultRaw&& other) HADESMEM_NOEXCEPT;

  ~CallResultRaw();

  DWORD GetLastError() const HADESMEM_NOEXCEPT;
  
  template <typename T>
  typename std::decay<T>::type GetReturnValue() const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::decay<T>::type>::value || 
      std::is_same<double, typename std::decay<T>::type>::value);

    typedef typename std::decay<T>::type U;
    return GetReturnValueImpl<U>(std::is_pointer<U>());
  }
  
private:
  template <typename T>
  T GetReturnValueIntImpl(std::true_type) const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::is_integral<T>::value);
    return static_cast<T>(GetReturnValueInt64());
  }

  template <typename T>
  T GetReturnValueIntImpl(std::false_type) const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(sizeof(T) <= sizeof(DWORD32));
    HADESMEM_STATIC_ASSERT(std::is_integral<T>::value);
    return static_cast<T>(GetReturnValueInt32());
  }

  template <typename T>
  T GetReturnValuePtrImpl(std::true_type) const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::is_pointer<T>::value);
    return reinterpret_cast<T>(GetReturnValueInt64());
  }

  template <typename T>
  T GetReturnValuePtrImpl(std::false_type) const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::is_pointer<T>::value);
    return reinterpret_cast<T>(GetReturnValueInt32());
  }

  template <typename T>
  auto GetReturnValueIntOrFloatImpl() const HADESMEM_NOEXCEPT 
    -> typename std::enable_if<std::is_same<float, T>::value, T>::type
  {
    return GetReturnValueFloat();
  }

  template <typename T>
  auto GetReturnValueIntOrFloatImpl() const HADESMEM_NOEXCEPT 
    -> typename std::enable_if<std::is_same<double, T>::value, T>::type
  {
    return GetReturnValueDouble();
  }

  template <typename T>
  auto GetReturnValueIntOrFloatImpl() const HADESMEM_NOEXCEPT 
    -> typename std::enable_if<std::is_integral<T>::value, T>::type
  {
    return GetReturnValueIntImpl<T>(std::integral_constant<bool, 
      (sizeof(T) == sizeof(DWORD64))>());
  }

  template <typename T>
  T GetReturnValueImpl(std::true_type) const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::is_pointer<T>::value);
    return GetReturnValuePtrImpl<T>(std::integral_constant<bool, 
      (sizeof(void*) == sizeof(DWORD64))>());
  }

  template <typename T>
  T GetReturnValueImpl(std::false_type) const HADESMEM_NOEXCEPT
  {
    HADESMEM_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_floating_point<T>::value);
    return GetReturnValueIntOrFloatImpl<T>();
  }

  DWORD_PTR GetReturnValueIntPtr() const HADESMEM_NOEXCEPT;

  DWORD32 GetReturnValueInt32() const HADESMEM_NOEXCEPT;

  DWORD64 GetReturnValueInt64() const HADESMEM_NOEXCEPT;

  float GetReturnValueFloat() const HADESMEM_NOEXCEPT;

  double GetReturnValueDouble() const HADESMEM_NOEXCEPT;

  struct Impl;
  std::unique_ptr<Impl> impl_;
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
    : arg_()
  {
    HADESMEM_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::remove_cv<T>::type>::value || 
      std::is_same<double, typename std::remove_cv<T>::type>::value);
    
    Initialize(t);
  }
  
  boost::variant<DWORD32, DWORD64, float, double> GetVariant() const
  {
    return arg_;
  }
  
private:
  template <typename T>
  void Initialize(T t) HADESMEM_NOEXCEPT
  {
    typedef typename std::conditional<sizeof(T) == sizeof(DWORD64), DWORD64, 
      DWORD32>::type D;
    Initialize(static_cast<D>(t));
  }

  template <typename T>
  void Initialize(T const* t) HADESMEM_NOEXCEPT
  {
    typedef typename std::conditional<sizeof(T const*) == sizeof(DWORD64), 
      DWORD64, DWORD32>::type D;
    Initialize(reinterpret_cast<D>(t));
  }

  template <typename T>
  void Initialize(T* t) HADESMEM_NOEXCEPT
  {
    Initialize(static_cast<T const*>(t));
  }

  void Initialize(DWORD32 t) HADESMEM_NOEXCEPT
  {
    arg_ = t;
  }

  void Initialize(DWORD64 t) HADESMEM_NOEXCEPT
  {
    arg_ = t;
  }
  
  void Initialize(float t) HADESMEM_NOEXCEPT
  {
    arg_ = t;
  }
  
  void Initialize(double t) HADESMEM_NOEXCEPT
  {
    arg_ = t;
  }
  
  typedef boost::variant<DWORD32, DWORD64, float, double> Arg;
  Arg arg_;
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
  explicit MultiCall(Process const& process);
  
  MultiCall(MultiCall const& other);
  
  MultiCall& operator=(MultiCall const& other);
  
  MultiCall(MultiCall&& other) HADESMEM_NOEXCEPT;
  
  MultiCall& operator=(MultiCall&& other) HADESMEM_NOEXCEPT;

  ~MultiCall();

#ifndef HADESMEM_NO_VARIADIC_TEMPLATES

  template <typename FuncT, typename... Args>
  void Add(FnPtr address, CallConv call_conv, Args&&... args)
  {
    HADESMEM_STATIC_ASSERT(detail::FuncArity<FuncT>::value == sizeof...(args));

    std::vector<CallArg> call_args;
    call_args.reserve(sizeof...(args));
    detail::BuildCallArgs<FuncT, 0>(&call_args, args...);

    AddImpl(address, call_conv, call_args);
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
    AddImpl(address, call_conv, args);\
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
  void AddImpl(FnPtr address, CallConv call_conv, 
    std::vector<CallArg> const& args);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
