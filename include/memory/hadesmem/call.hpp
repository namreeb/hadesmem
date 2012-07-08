// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <vector>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/any.hpp>
#include <boost/mpl/at.hpp>
#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <AsmJit/X86.h>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

class Process;

enum class CallConv
{
  kDefault, 
  kFastcall, 
  kStdcall, 
  kCdecl
};

// std::vector<boost::any> const& args

template <typename FuncT>
typename boost::function_types::result_type<FuncT>::type Call(Process const& process, CallConv call_conv, PVOID address, 
  typename boost::mpl::at<boost::function_types::parameter_types<FuncT>, boost::mpl::int_<0>>::type arg0, 
  typename boost::mpl::at<boost::function_types::parameter_types<FuncT>, boost::mpl::int_<1>>::type arg1)
{
  typedef typename boost::function_types::result_type<FuncT>::type ResultT;
  typedef boost::function_types::parameter_types<FuncT> ArgsT;
  typedef typename boost::mpl::at<ArgsT, boost::mpl::int_<0>>::type Arg0T;
  typedef typename boost::mpl::at<ArgsT, boost::mpl::int_<1>>::type Arg1T;
  
  uint32_t asmjit_call_conv = 0;
  switch (call_conv)
  {
  case CallConv::kDefault:
    asmjit_call_conv = AsmJit::kX86FuncConvDefault;
    break;
  case CallConv::kFastcall:
    asmjit_call_conv = AsmJit::kX86FuncConvCompatFastCall;
    break;
  case CallConv::kStdcall:
    asmjit_call_conv = AsmJit::kX86FuncConvStdCall;
    break;
  case CallConv::kCdecl:
    asmjit_call_conv = AsmJit::kX86FuncConvCDecl;
    break;
  default:
    BOOST_ASSERT(false);
  }
  
  AsmJit::X86Compiler c;
  c.newFunc(asmjit_call_conv, AsmJit::FuncBuilder2<ResultT, Arg0T, Arg1T>());
  //compiler.getFunc()->setHint(kFuncHintNaked, true);
  
  AsmJit::GpVar a0(c.newGpVar());
  c.mov(a0, AsmJit::imm(arg0));
  AsmJit::GpVar a1(c.newGpVar());
  c.mov(a1, AsmJit::imm(arg1));
  AsmJit::GpVar r0(c.newGpVar());
  
  AsmJit::X86CompilerFuncCall* ctx = c.call(address);
  AsmJit::FuncBuilderX func_proto;
  func_proto.setReturnTypeT<ResultT>();
  func_proto.addArgumentT<Arg0T>();
  func_proto.addArgumentT<Arg1T>();
  ctx->setPrototype(asmjit_call_conv, func_proto);
  ctx->setArgument(0, a0);
  ctx->setArgument(1, a1);
  ctx->setReturn(r0);
  
  c.ret(r0);
  c.endFunc();
  
  void* func_local = c.make();
  
  FuncT func_local_real = asmjit_cast<FuncT>(func_local);
  
  auto ret_val = func_local_real(arg0, arg1);
  
  return ret_val;
}

}
