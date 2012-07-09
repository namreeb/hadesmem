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
#include <boost/function_types/function_arity.hpp>
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
    asmjit_call_conv = AsmJit::kX86FuncConvCompatStdCall;
    break;
  case CallConv::kCdecl:
    asmjit_call_conv = AsmJit::kX86FuncConvCompatCDecl;
    break;
  default:
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Invalid calling convention."));
  }
  
  int const num_args = boost::function_types::function_arity<FuncT>::value;
  /*if (args.size() != num_args)
  {
    BOOST_THROW_EXCEPTION(HadesError() << 
      ErrorString("Invalid number of arguments."));
  }*/
  
  static_assert(std::is_integral<ResultT>::value, "Unsupported result type.");
  static_assert(sizeof(ResultT) == sizeof(PVOID), "Unsupported result size.");
  
  ResultT ret_val;
  
  AsmJit::X86Compiler c;
  c.newFunc(AsmJit::kX86FuncConvCompatStdCall, AsmJit::FuncBuilder1<DWORD, LPVOID>());
  //c.getFunc()->setHint(kFuncHintNaked, true);
  
  std::vector<AsmJit::GpVar> asmjit_call_args;
  asmjit_call_args.emplace_back(c.newGpVar());
  c.mov(asmjit_call_args[0], AsmJit::imm(arg0));
  asmjit_call_args.emplace_back(c.newGpVar());
  c.mov(asmjit_call_args[1], AsmJit::imm(arg1));
  
  AsmJit::GpVar r0(c.newGpVar());
  
  AsmJit::X86CompilerFuncCall* ctx = c.call(address);
  AsmJit::FuncBuilderX func_proto;
  func_proto.setReturnTypeT<ResultT>();
  func_proto.addArgumentT<Arg0T>();
  func_proto.addArgumentT<Arg1T>();
  ctx->setPrototype(asmjit_call_conv, func_proto);
  ctx->setArgument(0, asmjit_call_args[0]);
  ctx->setArgument(1, asmjit_call_args[1]);
  ctx->setReturn(r0);
  
  c.mov(AsmJit::sysint_ptr_abs(&ret_val), r0);
  
  AsmJit::GpVar r1(c.newGpVar(AsmJit::kX86VarTypeGpd));
  c.mov(r1, AsmJit::Imm(0));
  
  c.ret(r1);
  c.endFunc();
  
  void* func_local = c.make();
  
  LPTHREAD_START_ROUTINE func_local_real = asmjit_cast<LPTHREAD_START_ROUTINE>(func_local);
  
  // Why the hell doesn't the return value work?
  /*DWORD test_ret = */func_local_real(nullptr);
  //BOOST_ASSERT(test_ret == 0);
  
  return ret_val;
}

}
