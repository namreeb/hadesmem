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

// TODO: Clean up style.
// TODO: Support 'long double' return values.
class RemoteFunctionRet
{
public:
  RemoteFunctionRet(DWORD_PTR ReturnValue, DWORD64 ReturnValue64, 
    float ReturnValueFloat, double ReturnValueDouble, DWORD LastError);
  
  DWORD_PTR GetReturnValue() const;
  
  DWORD64 GetReturnValue64() const;
  
  float GetReturnValueFloat() const;
  
  double GetReturnValueDouble() const;
  
  DWORD GetLastError() const;
  
private:
  DWORD_PTR m_ReturnValue;
  DWORD64 m_ReturnValue64;
  float m_ReturnValueFloat;
  double m_ReturnValueDouble;
  DWORD m_LastError;
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

RemoteFunctionRet Call(Process const& process, 
  LPCVOID address, 
  CallConv call_conv, 
  std::vector<PVOID> const& args);

std::vector<RemoteFunctionRet> CallMulti(Process const& process, 
  std::vector<LPCVOID> addresses, 
  std::vector<CallConv> call_convs, 
  std::vector<std::vector<PVOID>> const& args_full);

// TODO: Improve and clean up this mess, move to different file, etc.

template <typename FuncT>
RemoteFunctionRet Call(Process const& process, 
  LPCVOID address, 
  CallConv call_conv)
{
  static_assert(boost::function_types::function_arity<FuncT>::value == 0, "Invalid number of arguments.");
  return Call(process, address, call_conv, std::vector<PVOID>());
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
static_assert(std::is_integral<A##n>::value || std::is_pointer<A##n>::value, "Currently only integral or pointer types are supported.");\
static_assert(sizeof(A##n) <= sizeof(PVOID), "Currently only memsize (or smaller) types are supported.");\
static_assert(std::is_convertible<T##n, A##n>::value, "Can not convert argument to type specified in function prototype.");\
union U##n { A##n a; PVOID p; } u##n;\
u##n.a = static_cast<A##n>(t##n);\
args.push_back(u##n.p);\

#define BOOST_PP_LOCAL_MACRO(n)\
template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
RemoteFunctionRet Call(Process const& process, LPCVOID address, CallConv call_conv \
  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T,&& t))\
{\
  static_assert(boost::function_types::function_arity<FuncT>::value == n, "Invalid number of arguments.");\
  std::vector<PVOID> args;\
  BOOST_PP_REPEAT(n, HADESMEM_CALL_ADD_ARG, ~)\
  return Call(process, address, call_conv, args);\
}\

#define BOOST_PP_LOCAL_LIMITS (1, HADESMEM_CALL_MAX_ARGS)

#include BOOST_PP_LOCAL_ITERATE()

#undef HADESMEM_CALL_DEFINE_ARG

#undef HADESMEM_CALL_ADD_ARG

}
