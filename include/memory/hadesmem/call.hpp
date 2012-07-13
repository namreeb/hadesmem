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

class RemoteFunctionRet
{
public:
  RemoteFunctionRet(DWORD_PTR ReturnValue, DWORD64 ReturnValue64, 
    DWORD LastError);
  
  DWORD_PTR GetReturnValue() const;
  
  DWORD64 GetReturnValue64() const;
  
  DWORD GetLastError() const;
  
private:
  DWORD_PTR m_ReturnValue;
  DWORD64 m_ReturnValue64;
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

#ifndef HADESMEM_CALL_MAX_ARGS
#define HADESMEM_CALL_MAX_ARGS 10
#endif

#if HADESMEM_CALL_MAX_ARGS > BOOST_PP_LIMIT_REPEAT
#error "[HadesMem] HADESMEM_CALL_MAX_ARGS exceeds Boost.Preprocessor repeat limit."
#endif

#if HADESMEM_CALL_MAX_ARGS > BOOST_PP_LIMIT_ITERATION
#error "[HadesMem] HADESMEM_CALL_MAX_ARGS exceeds Boost.Preprocessor iteration limit."
#endif

#define HADESMEM_CALL_DEFINE_ARG(z, n, unused) , typename boost::mpl::at_c<boost::function_types::parameter_types<FuncT>, n>::type a##n

#define HADESMEM_CALL_ADD_ARG(z, n, unused) \
typedef typename boost::mpl::at_c<boost::function_types::parameter_types<FuncT>, n>::type A##n;\
static_assert(std::is_integral<A##n>::value || std::is_pointer<A##n>::value, "Currently only integral or pointer types are supported.");\
static_assert(sizeof(A##n) <= sizeof(PVOID), "Currently only memsize (or smaller) types are supported.");\
args.push_back((PVOID)(a##n));\

#define BOOST_PP_LOCAL_MACRO(n)\
template <typename FuncT>\
RemoteFunctionRet Call(Process const& process, LPCVOID address, CallConv call_conv \
  BOOST_PP_REPEAT(n, HADESMEM_CALL_DEFINE_ARG, ~))\
{\
  typedef typename boost::function_types::result_type<FuncT>::type ResultT;\
  static_assert(boost::function_types::function_arity<FuncT>::value == n, "Invalid number of arguments.");\
  std::vector<PVOID> args;\
  BOOST_PP_REPEAT(n, HADESMEM_CALL_ADD_ARG, ~)\
  typedef typename boost::function_types::result_type<FuncT>::type ResultT;\
  return Call(process, address, call_conv, args);\
}\

#define BOOST_PP_LOCAL_LIMITS (1, HADESMEM_CALL_MAX_ARGS)

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#endif // #if defined(HADESMEM_GCC)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

#undef HADESMEM_CALL_DEFINE_ARG

#undef HADESMEM_CALL_ADD_ARG

}
