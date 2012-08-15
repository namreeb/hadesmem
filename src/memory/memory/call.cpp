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

// TODO: Improve and clean up this mess, move details to different files, 
// split code gen into detail funcs, etc.

// TODO: Split code gen into detail funcs etc.

// TODO: BOOST_NOEXCEPT annotations.

// TODO: Long double support.

// TODO: Support return values larger than 64-bits (via the 'secret' first 
// parameter).

// TODO: Support parameters larger than 64-bits (passed via address).

// TODO: Improve safety via EH.

// TODO: Support 'void' return type.

// TODO: Thiscall improvements. (Sanity check that first arg is a pointer type
// etc.)

// TODO: Consolidate return value allocations.

// TODO: Support function pointers and member-function pointers.

// TODO: Improve genericity of return value handling.

namespace hadesmem
{

RemoteFunctionRet::RemoteFunctionRet(DWORD_PTR return_int_ptr, 
  DWORD64 return_int_64, float return_float, double return_double, 
  DWORD last_error)
  : int_ptr_(return_int_ptr), 
  int_64_(return_int_64), 
  float_(return_float), 
  double_(return_double), 
  last_error_(last_error)
{ }

DWORD_PTR RemoteFunctionRet::GetReturnValue() const
{
  return int_ptr_;
}

DWORD64 RemoteFunctionRet::GetReturnValue64() const
{
  return int_64_;
}

float RemoteFunctionRet::GetReturnValueFloat() const
{
  return float_;
}

double RemoteFunctionRet::GetReturnValueDouble() const
{
  return double_;
}

DWORD RemoteFunctionRet::GetLastError() const
{
  return last_error_;
}

RemoteFunctionRet Call(Process const& process, 
  LPCVOID address, 
  CallConv call_conv, 
  std::vector<CallArg> const& args)
{
  std::vector<LPCVOID> addresses;
  addresses.push_back(address);
  std::vector<CallConv> call_convs;
  call_convs.push_back(call_conv);
  std::vector<std::vector<CallArg>> args_full;
  args_full.push_back(args);
  return CallMulti(process, addresses, call_convs, args_full)[0];
}

#if defined(_M_AMD64)

class X64ArgVisitor
{
public:
  X64ArgVisitor(AsmJit::X86Assembler* assembler, std::size_t num_args)
    : assembler_(assembler), 
    num_args_(num_args), 
    cur_arg_(num_args)
  { }
  
  void operator()(void* arg)
  {
    switch (cur_arg_)
    {
    case 1:
      assembler_->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(arg));
      break;
    case 2:
      assembler_->mov(AsmJit::rdx, reinterpret_cast<DWORD_PTR>(arg));
      break;
    case 3:
      assembler_->mov(AsmJit::r8, reinterpret_cast<DWORD_PTR>(arg));
      break;
    case 4:
      assembler_->mov(AsmJit::r9, reinterpret_cast<DWORD_PTR>(arg));
      break;
    default:
      assembler_->mov(AsmJit::rax, reinterpret_cast<DWORD_PTR>(arg));
      assembler_->mov(AsmJit::qword_ptr(AsmJit::rsp, cur_arg_ * 8 - 8), 
        AsmJit::rax);
      break;
    }
    
    --cur_arg_;
  }
  
  void operator()(float arg)
  {
    static_assert(sizeof(float) == sizeof(DWORD), "Invalid type sizes.");
    
    union FloatConv
    {
      float f;
      DWORD i;
    };
        
    FloatConv float_conv;
    float_conv.f = arg;
    
    switch (cur_arg_)
    {
    case 1:
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8), 
        static_cast<DWORD>(float_conv.i));
      assembler_->movss(AsmJit::xmm0, AsmJit::dword_ptr(AsmJit::rsp, 
        num_args_ * 8));
      break;
    case 2:
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8), 
        static_cast<DWORD>(float_conv.i));
      assembler_->movss(AsmJit::xmm1, AsmJit::dword_ptr(AsmJit::rsp, 
        num_args_ * 8));
      break;
    case 3:
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8), 
        static_cast<DWORD>(float_conv.i));
      assembler_->movss(AsmJit::xmm2, AsmJit::dword_ptr(AsmJit::rsp, 
        num_args_ * 8));
      break;
    case 4:
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8), 
        static_cast<DWORD>(float_conv.i));
      assembler_->movss(AsmJit::xmm3, AsmJit::dword_ptr(AsmJit::rsp, 
        num_args_ * 8));
      break;
    default:
      assembler_->xor_(AsmJit::rax, AsmJit::rax);
      assembler_->mov(AsmJit::rax, float_conv.i);
      assembler_->mov(AsmJit::qword_ptr(AsmJit::rsp, cur_arg_ * 8 - 8), 
        AsmJit::rax);
      break;
    }
    
    --cur_arg_;
  }
  
  void operator()(double arg)
  {
    static_assert(sizeof(double) == sizeof(DWORD64), "Invalid type sizes.");
    
    union DoubleConv
    {
      double d;
      DWORD64 i;
    };
    
    DoubleConv double_conv;
    double_conv.d = arg;
    
    switch (cur_arg_)
    {
    case 1:
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8), 
        static_cast<DWORD>(double_conv.i));
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8 + 4), 
        static_cast<DWORD>((double_conv.i >> 32) & 0xFFFFFFFF));
      assembler_->movsd(AsmJit::xmm0, AsmJit::qword_ptr(AsmJit::rsp, 
        num_args_ * 8));
      break;
    case 2:
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8), 
        static_cast<DWORD>(double_conv.i));
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8 + 4), 
        static_cast<DWORD>((double_conv.i >> 32) & 0xFFFFFFFF));
      assembler_->movsd(AsmJit::xmm1, AsmJit::qword_ptr(AsmJit::rsp, 
        num_args_ * 8));
      break;
    case 3:
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8), 
        static_cast<DWORD>(double_conv.i));
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8 + 4), 
        static_cast<DWORD>((double_conv.i >> 32) & 0xFFFFFFFF));
      assembler_->movsd(AsmJit::xmm2, AsmJit::qword_ptr(AsmJit::rsp, 
        num_args_ * 8));
      break;
    case 4:
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8), 
        static_cast<DWORD>(double_conv.i));
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, num_args_ * 8 + 4), 
        static_cast<DWORD>((double_conv.i >> 32) & 0xFFFFFFFF));
      assembler_->movsd(AsmJit::xmm3, AsmJit::qword_ptr(AsmJit::rsp, 
        num_args_ * 8));
      break;
    default:
      assembler_->mov(AsmJit::rax, double_conv.i);
      assembler_->mov(AsmJit::qword_ptr(AsmJit::rsp, cur_arg_ * 8 - 8), 
        AsmJit::rax);
      break;
    }
    
    --cur_arg_;
  }
  
  void operator()(DWORD64 /*arg*/)
  {
    BOOST_ASSERT("Invalid argument type." && false);
  }
  
private:
  AsmJit::X86Assembler* assembler_;
  std::size_t num_args_;
  std::size_t cur_arg_;
};

#elif defined(_M_IX86)

class X86ArgVisitor
{
public:
  X86ArgVisitor(AsmJit::X86Assembler* assembler, std::size_t num_args, 
    CallConv call_conv)
    : assembler_(assembler), 
    num_args_(num_args), 
    cur_arg_(num_args), 
    call_conv_(call_conv)
  { }
  
  void operator()(void* arg)
  {
    switch (cur_arg_)
    {
    case 1:
      switch (call_conv_)
      {
      case CallConv::kThisCall:
      case CallConv::kFastCall:
        assembler_->mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(arg));
        break;
      default:
        assembler_->mov(AsmJit::eax, reinterpret_cast<DWORD_PTR>(arg));
        assembler_->push(AsmJit::eax);
        break;
      }
      break;
    case 2:
      switch (call_conv_)
      {
      case CallConv::kFastCall:
        assembler_->mov(AsmJit::edx, reinterpret_cast<DWORD_PTR>(arg));
        break;
      default:
        assembler_->mov(AsmJit::eax, reinterpret_cast<DWORD_PTR>(arg));
        assembler_->push(AsmJit::eax);
        break;
      }
      break;
    default:
      assembler_->mov(AsmJit::eax, reinterpret_cast<DWORD_PTR>(arg));
      assembler_->push(AsmJit::eax);
      break;
    }
    
    --cur_arg_;
  }
  
  void operator()(float arg)
  {
    static_assert(sizeof(float) == sizeof(DWORD), "Invalid type sizes.");
    
    union FloatConv
    {
      float f;
      DWORD i;
    };
        
    FloatConv float_conv;
    float_conv.f = arg;
    
    assembler_->mov(AsmJit::eax, float_conv.i);
    assembler_->push(AsmJit::eax);
    
    --cur_arg_;
  }
  
  void operator()(double arg)
  {
    static_assert(sizeof(double) == sizeof(DWORD64), "Invalid type sizes.");
    
    union DoubleConv
    {
      double d;
      DWORD64 i;
    };
    
    DoubleConv double_conv;
    double_conv.d = arg;
    
    assembler_->mov(AsmJit::eax, static_cast<DWORD>((double_conv.i >> 32) & 
      0xFFFFFFFF));
    assembler_->push(AsmJit::eax);
    
    assembler_->mov(AsmJit::eax, static_cast<DWORD>(double_conv.i));
    assembler_->push(AsmJit::eax);
    
    --cur_arg_;
  }
  
  void operator()(DWORD64 arg)
  {
    assembler_->mov(AsmJit::eax, static_cast<DWORD>((arg >> 32) & 
      0xFFFFFFFF));
    assembler_->push(AsmJit::eax);
    
    assembler_->mov(AsmJit::eax, static_cast<DWORD>(arg));
    assembler_->push(AsmJit::eax);
    
    --cur_arg_;
  }
  
private:
  AsmJit::X86Assembler* assembler_;
  std::size_t num_args_;
  std::size_t cur_arg_;
  CallConv call_conv_;
};

#else
#error "[HadesMem] Unsupported architecture."
#endif

std::vector<RemoteFunctionRet> CallMulti(Process const& process, 
  std::vector<LPCVOID> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full) 
{
  if (addresses.size() != call_convs.size() || 
    addresses.size() != args_full.size())
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Size mismatch in parameters."));
  }
  
  Allocator const return_value_remote(&process, sizeof(DWORD_PTR) * 
    addresses.size());
  Allocator const return_value_64_remote(&process, sizeof(DWORD64) * 
    addresses.size());
  Allocator const return_value_float_remote(&process, sizeof(float) * 
    addresses.size());
  Allocator const return_value_double_remote(&process, sizeof(double) * 
    addresses.size());
  Allocator const last_error_remote(&process, sizeof(DWORD) * 
    addresses.size());

  Module kernel32(&process, L"kernel32.dll");
  DWORD_PTR const get_last_error = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "GetLastError"));
  DWORD_PTR const set_last_error = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "SetLastError"));
  DWORD_PTR const is_debugger_present = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "IsDebuggerPresent"));
  DWORD_PTR const debug_break = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "DebugBreak"));
  
  AsmJit::X86Assembler assembler;
  
  AsmJit::Label label_nodebug(assembler.newLabel());
  
#if defined(_M_AMD64)
  using std::begin;
  using std::end;
  auto const max_args_list = std::max_element(begin(args_full), 
    end(args_full), 
    [] (std::vector<CallArg> const& args1, std::vector<CallArg> const& args2)
    {
      return args1.size() < args2.size();
    });
  std::size_t const max_num_args = max_args_list->size();
  
  std::size_t stack_offs = (std::max)(static_cast<std::size_t>(0x20), 
    max_num_args * 0x8);
  BOOST_ASSERT(stack_offs % 16 == 0 || stack_offs % 16 == 8);
  stack_offs = (stack_offs % 16) ? (stack_offs + 8) : stack_offs;
  stack_offs += 16;
  stack_offs += 8;
  
  assembler.sub(AsmJit::rsp, stack_offs);
  
  assembler.mov(AsmJit::rax, is_debugger_present);
  assembler.call(AsmJit::rax);
  
  assembler.test(AsmJit::rax, AsmJit::rax);
  assembler.jz(label_nodebug);
  
  assembler.mov(AsmJit::rax, debug_break);
  assembler.call(AsmJit::rax);
  
  assembler.bind(label_nodebug);
  
  assembler.mov(AsmJit::rcx, 0);
  assembler.mov(AsmJit::rax, set_last_error);
  assembler.call(AsmJit::rax);
  
  for (std::size_t i = 0; i < addresses.size(); ++i)
  {
    LPCVOID address = addresses[i];
    CallConv call_conv = call_convs[i];
    std::vector<CallArg> const& args = args_full[i];
    std::size_t const num_args = args.size();
    
    if (call_conv != CallConv::kX64 && 
      call_conv != CallConv::kDefault)
    {
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("Invalid calling convention."));
    }
    
    X64ArgVisitor arg_visitor(&assembler, num_args);
    std::for_each(args.rbegin(), args.rend(), 
      [&] (CallArg const& arg)
      {
        arg.Visit(&arg_visitor);
      });
    
    assembler.mov(AsmJit::rax, reinterpret_cast<DWORD_PTR>(address));
    assembler.call(AsmJit::rax);
    
    assembler.mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_value_remote.GetBase()) + i * sizeof(DWORD_PTR));
    assembler.mov(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::rax);
    
    assembler.mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_value_64_remote.GetBase()) + i * sizeof(DWORD64));
    assembler.mov(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::rax);
    
    assembler.mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_value_float_remote.GetBase()) + i * sizeof(float));
    assembler.movd(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::xmm0);
    
    assembler.mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_value_double_remote.GetBase()) + i * sizeof(double));
    assembler.movq(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::xmm0);
    
    assembler.mov(AsmJit::rax, get_last_error);
    assembler.call(AsmJit::rax);
    
    assembler.mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      last_error_remote.GetBase()) + i * sizeof(DWORD));
    assembler.mov(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::rax);
  }
  
  assembler.add(AsmJit::rsp, stack_offs);
  
  assembler.ret();
#elif defined(_M_IX86)
  
  assembler.push(AsmJit::ebp);
  assembler.mov(AsmJit::ebp, AsmJit::esp);
  
  assembler.mov(AsmJit::eax, is_debugger_present);
  assembler.call(AsmJit::eax);
  
  assembler.test(AsmJit::eax, AsmJit::eax);
  assembler.jz(label_nodebug);
  
  assembler.mov(AsmJit::eax, debug_break);
  assembler.call(AsmJit::eax);
  
  assembler.bind(label_nodebug);
  
  assembler.push(AsmJit::imm(0x0));
  assembler.mov(AsmJit::eax, set_last_error);
  assembler.call(AsmJit::eax);
  
  for (std::size_t i = 0; i < addresses.size(); ++i)
  {
    LPCVOID address = addresses[i];
    CallConv call_conv = call_convs[i];
    std::vector<CallArg> const& args = args_full[i];
    std::size_t const num_args = args.size();
    
    X86ArgVisitor arg_visitor(&assembler, num_args, call_conv);
    std::for_each(args.rbegin(), args.rend(), 
      [&] (CallArg const& arg)
      {
        arg.Visit(&arg_visitor);
      });
    
    assembler.mov(AsmJit::eax, reinterpret_cast<DWORD_PTR>(address));
    assembler.call(AsmJit::eax);
    
    assembler.mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_value_remote.GetBase()) + i * sizeof(DWORD_PTR));
    assembler.mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);
    
    assembler.mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_value_64_remote.GetBase()) + i * sizeof(DWORD64));
    assembler.mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);
    assembler.mov(AsmJit::dword_ptr(AsmJit::ecx, 4), AsmJit::edx);
    
    assembler.mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_value_float_remote.GetBase()) + i * sizeof(float));
    assembler.fst(AsmJit::dword_ptr(AsmJit::ecx));
    
    assembler.mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_value_double_remote.GetBase()) + i * sizeof(double));
    assembler.fst(AsmJit::qword_ptr(AsmJit::ecx));
    
    assembler.mov(AsmJit::eax, get_last_error);
    assembler.call(AsmJit::eax);
    
    assembler.mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      last_error_remote.GetBase()) + i * sizeof(DWORD));
    assembler.mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);
    
    if (call_conv == CallConv::kCdecl)
    {
      assembler.add(AsmJit::esp, AsmJit::imm(num_args * sizeof(PVOID)));
    }
  }
  
  assembler.mov(AsmJit::esp, AsmJit::ebp);
  assembler.pop(AsmJit::ebp);
  
  assembler.ret(AsmJit::imm(0x4));
#else
#error "[HadesMem] Unsupported architecture."
#endif
  
  DWORD_PTR const stub_size = assembler.getCodeSize();
  
  Allocator const stub_mem_remote(&process, stub_size);
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
  
  std::vector<RemoteFunctionRet> return_vals;
  for (std::size_t i = 0; i < addresses.size(); ++i)
  {
    DWORD_PTR const ret_val = Read<DWORD_PTR>(process, static_cast<DWORD_PTR*>(
      return_value_remote.GetBase()) + i);
    DWORD64 const ret_val_64 = Read<DWORD64>(process, static_cast<DWORD64*>(
      return_value_64_remote.GetBase()) + i);
    float const ret_val_float = Read<float>(process, static_cast<float*>(
      return_value_float_remote.GetBase()) + i);
    double const ret_val_double = Read<double>(process, static_cast<double*>(
      return_value_double_remote.GetBase()) + i);
    DWORD const error_code = Read<DWORD>(process, static_cast<DWORD*>(
      last_error_remote.GetBase()) + i);
    return_vals.push_back(RemoteFunctionRet(ret_val, ret_val_64, 
      ret_val_float, ret_val_double, error_code));
  }
  
  return return_vals;
}

}
