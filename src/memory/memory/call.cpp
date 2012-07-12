// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/call.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/scope_exit.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <AsmJit/AsmJit.h>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/read.hpp"
#include "hadesmem/alloc.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"

// TODO: Rewrite, clean up, etc...
// TODO: Split code gen into detail funcs etc.

namespace hadesmem
{

RemoteFunctionRet::RemoteFunctionRet(DWORD_PTR ReturnValue, DWORD LastError)
  : m_ReturnValue(ReturnValue), 
  m_LastError(LastError)
{ }

DWORD_PTR RemoteFunctionRet::GetReturnValue() const
{
  return m_ReturnValue;
}

DWORD RemoteFunctionRet::GetLastError() const
{
  return m_LastError;
}

RemoteFunctionRet Call(Process const& process, 
  LPCVOID address, 
  CallConv call_conv, 
  std::vector<PVOID> const& args)
{
  Allocator const return_value_remote(process, sizeof(DWORD_PTR));
  Allocator const last_error_remote(process, sizeof(DWORD));

  Module kernel32(&process, L"kernel32.dll");
  DWORD_PTR const get_last_error = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "GetLastError"));
  DWORD_PTR const set_last_error = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "SetLastError"));
  
  AsmJit::Assembler assembler;
  
  AsmJit::Compiler compiler;
  
  compiler.newFunction(AsmJit::CALL_CONV_COMPAT_STDCALL, AsmJit::FunctionBuilder1<int, int>());
  
  std::size_t const num_args = args.size();
  
  unsigned int asmjit_call_conv = 0;
  switch (call_conv)
  {
  case CallConv::kDefault:
    asmjit_call_conv = AsmJit::CALL_CONV_DEFAULT;
    break;
  case CallConv::kCdecl:
    asmjit_call_conv = AsmJit::CALL_CONV_COMPAT_CDECL;
    break;
  case CallConv::kStdCall:
    asmjit_call_conv = AsmJit::CALL_CONV_COMPAT_STDCALL;
    break;
  case CallConv::kThisCall:
    asmjit_call_conv = /*AsmJit::CALL_CONV_COMPAT_THISCALL*/ AsmJit::CALL_CONV_MSTHISCALL;
    break;
  case CallConv::kFastCall:
    asmjit_call_conv = AsmJit::CALL_CONV_COMPAT_FASTCALL;
    break;
  default:
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Invalid calling convention."));
  }
  
  AsmJit::GPVar set_last_error_arg(compiler.newGP());
  compiler.mov(set_last_error_arg, AsmJit::imm(0));
  AsmJit::GPVar set_last_error_var(compiler.newGP());
  compiler.mov(set_last_error_var, AsmJit::imm(set_last_error));
  AsmJit::ECall* set_last_error_call_context = compiler.call(set_last_error_var);
  set_last_error_call_context->setPrototype(AsmJit::CALL_CONV_COMPAT_STDCALL, AsmJit::FunctionBuilder1<AsmJit::Void, int>());
  set_last_error_call_context->setArgument(0, set_last_error_arg);
  
  AsmJit::GPVar target_var(compiler.newGP());
  AsmJit::GPVar target_return(compiler.newGP());
  compiler.mov(target_var, AsmJit::imm(reinterpret_cast<DWORD_PTR>(address)));
  std::vector<AsmJit::GPVar> target_call_args;
  for (std::size_t j = 0; j < num_args; ++j)
  {
    target_call_args.emplace_back(compiler.newGP());
    compiler.mov(target_call_args[j], AsmJit::imm(reinterpret_cast<DWORD_PTR>(args[j])));
  }
  AsmJit::ECall* target_call_context = compiler.call(target_var);
  AsmJit::FunctionBuilderX target_call_prototype;
  for (std::size_t j = 0; j < num_args; ++j)
  {
    target_call_prototype.addArgument<void*>();
  }
  target_call_prototype.setReturnValue<void*>();
  target_call_context->setPrototype(asmjit_call_conv, target_call_prototype);
  for (uint32_t j = 0; j < num_args; ++j)
  {
    target_call_context->setArgument(j, target_call_args[j]);
  }
  target_call_context->setReturn(target_return);
  AsmJit::GPVar return_value_out(compiler.newGP());
  compiler.mov(return_value_out, AsmJit::imm(reinterpret_cast<DWORD_PTR>(return_value_remote.GetBase())));
  compiler.mov(AsmJit::ptr(return_value_out), target_return);
  
  AsmJit::GPVar get_last_error_var(compiler.newGP());
  AsmJit::GPVar get_last_error_return(compiler.newGP());
  compiler.mov(get_last_error_var, AsmJit::imm(get_last_error));
  AsmJit::ECall* get_last_error_call_context = compiler.call(get_last_error_var);
  get_last_error_call_context->setPrototype(AsmJit::CALL_CONV_COMPAT_STDCALL, AsmJit::FunctionBuilder0<int>());
  get_last_error_call_context->setReturn(get_last_error_return);
  AsmJit::GPVar last_error_out(compiler.newGP());
  compiler.mov(last_error_out, AsmJit::imm(reinterpret_cast<DWORD_PTR>(last_error_remote.GetBase())));
  compiler.mov(AsmJit::ptr(last_error_out), get_last_error_return);
  
  compiler.endFunction();
  
  compiler.serialize(assembler);
  
  DWORD_PTR const stub_size = assembler.getCodeSize();
  BOOST_ASSERT(stub_size != 0);
  
  Allocator const stub_mem_remote(process, stub_size);
  PBYTE const stub_remote = static_cast<PBYTE>(stub_mem_remote.GetBase());
  DWORD_PTR const stub_remote_temp = reinterpret_cast<DWORD_PTR>(stub_remote);
  
  std::vector<BYTE> code_real(stub_size);
  assembler.relocCode(code_real.data(), reinterpret_cast<DWORD_PTR>(
    stub_remote));
  
  WriteVector(process, stub_remote, code_real);
  
  HANDLE const thread_remote = CreateRemoteThread(process.GetHandle(), 
    nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(stub_remote_temp), 
    nullptr, 0, nullptr);
  if (!thread_remote)
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not create remote thread.") << 
      ErrorCodeWinLast(last_error));
  }
  
  BOOST_SCOPE_EXIT_ALL(&)
  {
    // WARNING: Handle is leaked if FreeLibrary fails.
    BOOST_VERIFY(::CloseHandle(thread_remote));
  };
  
  if (WaitForSingleObject(thread_remote, INFINITE) != WAIT_OBJECT_0)
  {
    DWORD const LastError = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not wait for remote thread.") << 
      ErrorCodeWinLast(LastError));
  }
  
  DWORD_PTR const ret_val = Read<DWORD_PTR>(process, return_value_remote.GetBase());
  DWORD const error_code = Read<DWORD>(process, last_error_remote.GetBase());
  return RemoteFunctionRet(ret_val, error_code);
}

}
