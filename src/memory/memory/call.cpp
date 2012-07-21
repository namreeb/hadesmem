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

RemoteFunctionRet::RemoteFunctionRet(DWORD_PTR ReturnValue, 
  DWORD64 ReturnValue64, float ReturnValueFloat, double ReturnValueDouble, 
  DWORD LastError)
  : m_ReturnValue(ReturnValue), 
  m_ReturnValue64(ReturnValue64), 
  m_ReturnValueFloat(ReturnValueFloat), 
  m_ReturnValueDouble(ReturnValueDouble), 
  m_LastError(LastError)
{ }

DWORD_PTR RemoteFunctionRet::GetReturnValue() const
{
  return m_ReturnValue;
}

DWORD64 RemoteFunctionRet::GetReturnValue64() const
{
  return m_ReturnValue64;
}

float RemoteFunctionRet::GetReturnValueFloat() const
{
  return m_ReturnValueFloat;
}

double RemoteFunctionRet::GetReturnValueDouble() const
{
  return m_ReturnValueDouble;
}

DWORD RemoteFunctionRet::GetLastError() const
{
  return m_LastError;
}

RemoteFunctionRet Call(Process const& process, 
  LPCVOID address, 
  CallConv call_conv, 
  std::vector<boost::variant<PVOID, detail::WrappedFloat, detail::WrappedDouble>> const& args)
{
  std::vector<LPCVOID> addresses;
  addresses.push_back(address);
  std::vector<CallConv> call_convs;
  call_convs.push_back(call_conv);
  std::vector<std::vector<boost::variant<PVOID, detail::WrappedFloat, detail::WrappedDouble>>> args_full;
  args_full.push_back(args);
  return CallMulti(process, addresses, call_convs, args_full)[0];
}

class X64ArgParser : public boost::static_visitor<>
{
public:
  X64ArgParser(AsmJit::X86Assembler* assembler, std::size_t num_args)
    : assembler_(assembler), 
    num_args_(num_args)
  { }
  
  void operator()(PVOID arg)
  {
    switch (num_args_)
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
      assembler_->push(AsmJit::rax);
      break;
    }
    
    --num_args_;
  }
  
  void operator()(detail::WrappedFloat arg_wrapped)
  {
    float arg = arg_wrapped.f;
    union FloatConv
    {
      float f;
      DWORD i;
    };
        
    FloatConv float_conv;
    float_conv.f = arg;
    
    switch (num_args_)
    {
    case 1:
      assembler_->push(static_cast<DWORD>(float_conv.i));
      assembler_->movss(AsmJit::xmm0, AsmJit::dword_ptr(AsmJit::rsp));
      assembler_->add(AsmJit::rsp, 0x8);
      break;
    case 2:
      assembler_->push(static_cast<DWORD>(float_conv.i));
      assembler_->movss(AsmJit::xmm1, AsmJit::dword_ptr(AsmJit::rsp));
      assembler_->add(AsmJit::rsp, 0x8);
      break;
    case 3:
      assembler_->push(static_cast<DWORD>(float_conv.i));
      assembler_->movss(AsmJit::xmm2, AsmJit::dword_ptr(AsmJit::rsp));
      assembler_->add(AsmJit::rsp, 0x8);
      break;
    case 4:
      assembler_->push(static_cast<DWORD>(float_conv.i));
      assembler_->movss(AsmJit::xmm3, AsmJit::dword_ptr(AsmJit::rsp));
      assembler_->add(AsmJit::rsp, 0x8);
      break;
    default:
      assembler_->mov(AsmJit::eax, float_conv.i);
      assembler_->push(AsmJit::eax);
      break;
    }
    
    --num_args_;
  }
  
  void operator()(detail::WrappedDouble arg_wrapped)
  {
    double arg = arg_wrapped.d;
    union DoubleConv
    {
      double d;
      DWORD64 i;
    };
    
    DoubleConv double_conv;
    double_conv.d = arg;
    
    switch (num_args_)
    {
    case 1:
      assembler_->push(static_cast<DWORD>(double_conv.i));
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, 4), static_cast<DWORD>((double_conv.i >> 32) & 0xFFFFFFFF));
      assembler_->movsd(AsmJit::xmm0, AsmJit::qword_ptr(AsmJit::rsp));
      assembler_->add(AsmJit::rsp, 0x8);
      break;
    case 2:
      assembler_->push(static_cast<DWORD>(double_conv.i));
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, 4), static_cast<DWORD>((double_conv.i >> 32) & 0xFFFFFFFF));
      assembler_->movsd(AsmJit::xmm1, AsmJit::qword_ptr(AsmJit::rsp));
      assembler_->add(AsmJit::rsp, 0x8);
      break;
    case 3:
      assembler_->push(static_cast<DWORD>(double_conv.i));
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, 4), static_cast<DWORD>((double_conv.i >> 32) & 0xFFFFFFFF));
      assembler_->movsd(AsmJit::xmm2, AsmJit::qword_ptr(AsmJit::rsp));
      assembler_->add(AsmJit::rsp, 0x8);
      break;
    case 4:
      assembler_->push(static_cast<DWORD>(double_conv.i));
      assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, 4), static_cast<DWORD>((double_conv.i >> 32) & 0xFFFFFFFF));
      assembler_->movsd(AsmJit::xmm3, AsmJit::qword_ptr(AsmJit::rsp));
      assembler_->add(AsmJit::rsp, 0x8);
      break;
    default:
      assembler_->mov(AsmJit::rax, double_conv.i);
      assembler_->push(AsmJit::rax);
      break;
    }
    
    --num_args_;
  }
  
private:
  AsmJit::X86Assembler* assembler_;
  std::size_t num_args_;
};

// TODO: Investigate whether it's possible to use the AsmJit compiler to add 
// FP support after all...

// TODO: Ensure stack alignment is correct under x64 (should be 16-byte).

std::vector<RemoteFunctionRet> CallMulti(Process const& process, 
  std::vector<LPCVOID> addresses, 
  std::vector<CallConv> call_convs, 
  std::vector<std::vector<boost::variant<PVOID, detail::WrappedFloat, detail::WrappedDouble>>> const& args_full) 
{
  if (addresses.size() != call_convs.size() || 
    addresses.size() != args_full.size())
  {
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Size mismatch in parameters."));
  }
  
  Allocator const return_value_remote(process, sizeof(DWORD_PTR) * 
    addresses.size());
  Allocator const return_value_64_remote(process, sizeof(DWORD64) * 
    addresses.size());
  Allocator const return_value_float_remote(process, sizeof(float) * 
    addresses.size());
  Allocator const return_value_double_remote(process, sizeof(double) * 
    addresses.size());
  Allocator const last_error_remote(process, sizeof(DWORD) * 
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
  
  std::vector<AsmJit::Label> label_nodebug;
  
#if defined(_M_AMD64)
  for (std::size_t i = 0; i < addresses.size(); ++i)
  {
    LPCVOID address = addresses[i];
    CallConv call_conv = call_convs[i];
    std::vector<boost::variant<PVOID, detail::WrappedFloat, detail::WrappedDouble>> const& args = args_full[i];
    std::size_t const num_args = args.size();
    
    // Check calling convention
    if (call_conv != CallConv::kX64 && 
      call_conv != CallConv::kDefault)
    {
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("Invalid calling convention."));
    }
    
    // Prologue
    assembler.push(AsmJit::rbp);
    assembler.mov(AsmJit::rbp, AsmJit::rsp);
    
    // Call kernel32.dll!IsDebuggerPresent
    assembler.mov(AsmJit::rax, is_debugger_present);
    assembler.call(AsmJit::rax);
    
    // Call kernel32.dll!DebugBreak if IsDebuggerPresent returns TRUE
    label_nodebug.emplace_back(assembler.newLabel());
    assembler.test(AsmJit::rax, AsmJit::rax);
    assembler.jz(label_nodebug[i]);
    assembler.mov(AsmJit::rax, debug_break);
    assembler.call(AsmJit::rax);
    
    assembler.bind(label_nodebug[i]);
    
    // Allocate ghost space
    assembler.sub(AsmJit::rsp, AsmJit::Imm(0x20));

    // Call kernel32.dll!SetLastError
    assembler.mov(AsmJit::rcx, 0);
    assembler.mov(AsmJit::rax, set_last_error);
    assembler.call(AsmJit::rax);

    // Cleanup ghost space
    assembler.add(AsmJit::rsp, AsmJit::Imm(0x20));

    // Set up parameters
    X64ArgParser arg_parser(&assembler, num_args);
    std::for_each(args.rbegin(), args.rend(), boost::apply_visitor(arg_parser));

    // Allocate ghost space
    assembler.sub(AsmJit::rsp, AsmJit::Imm(0x20));

    // Call target
    assembler.mov(AsmJit::rax, reinterpret_cast<DWORD_PTR>(address));
    assembler.call(AsmJit::rax);
    
    // Cleanup ghost space
    assembler.add(AsmJit::rsp, AsmJit::Imm(0x20));

    // Clean up remaining stack space
    assembler.add(AsmJit::rsp, 0x8 * (num_args - 4));

    // Write return value to memory
    assembler.mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_value_remote.GetBase()) + i * sizeof(DWORD_PTR));
    assembler.mov(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::rax);

    // Write 64-bit return value to memory
    assembler.mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_value_64_remote.GetBase()) + i * sizeof(DWORD64));
    assembler.mov(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::rax);
    
    // Write float return value to memory
    assembler.mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(return_value_float_remote.GetBase()) + i * sizeof(float));
    assembler.movd(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::xmm0);
    
    // Write double return value to memory
    assembler.mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(return_value_double_remote.GetBase()) + i * sizeof(double));
    assembler.movq(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::xmm0);

    // Call kernel32.dll!GetLastError
    assembler.mov(AsmJit::rax, get_last_error);
    assembler.call(AsmJit::rax);
    
    // Write error code to memory
    assembler.mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      last_error_remote.GetBase()) + i * sizeof(DWORD));
    assembler.mov(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::rax);

    // Epilogue
    assembler.mov(AsmJit::rsp, AsmJit::rbp);
    assembler.pop(AsmJit::rbp);
  }

  // Return
  assembler.ret();
#elif defined(_M_IX86)
  for (std::size_t i = 0; i < addresses.size(); ++i)
  {
    LPCVOID address = addresses[i];
    CallConv call_conv = call_convs[i];
    std::vector<PVOID> const& args = args_full[i];
    std::size_t const num_args = args.size();
    
    // Prologue
    assembler.push(AsmJit::ebp);
    assembler.mov(AsmJit::ebp, AsmJit::esp);

    // Call kernel32.dll!SetLastError
    assembler.push(AsmJit::Imm(0x0));
    assembler.mov(AsmJit::eax, set_last_error);
    assembler.call(AsmJit::eax);

    // Get stack arguments offset
    std::size_t stack_arg_offs = 0;
    switch (call_conv)
    {
    case CallConv::kThisCall:
      stack_arg_offs = 1;
      break;

    case CallConv::kFastCall:
      stack_arg_offs = 2;
      break;

    case CallConv::kCdecl:
    case CallConv::kStdCall:
    case CallConv::kDefault:
      stack_arg_offs = 0;
      break;

    default:
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("Invalid calling convention."));
    }

    // Pass first arg in through ECX if 'thiscall' is specified
    if (call_conv == CallConv::kThisCall)
    {
      assembler.mov(AsmJit::ecx, num_args ? reinterpret_cast<DWORD_PTR>(
        args[0]) : 0);
    }

    // Pass first two args in through ECX and EDX if 'fastcall' is specified
    if (call_conv == CallConv::kFastCall)
    {
      assembler.mov(AsmJit::ecx, num_args ? reinterpret_cast<DWORD_PTR>(
        args[0]) : 0);
      assembler.mov(AsmJit::edx, num_args > 1 ? reinterpret_cast<DWORD_PTR>(
        args[1]) : 0);
    }

    // Pass all remaining args on stack if there are any left to process.
    if (num_args > stack_arg_offs)
    {
      std::for_each(args.crbegin(), args.crend() - stack_arg_offs, 
        [&] (PVOID arg)
      {
        assembler.mov(AsmJit::eax, reinterpret_cast<DWORD_PTR>(arg));
        assembler.push(AsmJit::eax);
      });
    }
    
    // Call target
    assembler.mov(AsmJit::eax, reinterpret_cast<DWORD_PTR>(address));
    assembler.call(AsmJit::eax);
    
    // Write return value to memory
    assembler.mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_value_remote.GetBase()) + i * sizeof(DWORD_PTR));
    assembler.mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);
    
    // Write 64-bit return value to memory
    assembler.mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_value_64_remote.GetBase()) + i * sizeof(DWORD64));
    assembler.mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);
    assembler.mov(AsmJit::dword_ptr(AsmJit::ecx, 4), AsmJit::edx);
    
    // Write float return value to memory
    assembler.mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(return_value_float_remote.GetBase()) + i * sizeof(float));
    assembler.fst(AsmJit::dword_ptr(AsmJit::ecx));
    
    // Write double return value to memory
    assembler.mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(return_value_double_remote.GetBase()) + i * sizeof(double));
    assembler.fst(AsmJit::qword_ptr(AsmJit::ecx));
    
    // Call kernel32.dll!GetLastError
    assembler.mov(AsmJit::eax, get_last_error);
    assembler.call(AsmJit::eax);
    
    // Write error code to memory
    assembler.mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      last_error_remote.GetBase()) + i * sizeof(DWORD));
    assembler.mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);
    
    // Clean up stack if necessary
    if (call_conv == CallConv::kCdecl)
    {
      assembler.add(AsmJit::esp, AsmJit::Imm(num_args * sizeof(PVOID)));
    }

    // Epilogue
    assembler.mov(AsmJit::esp, AsmJit::ebp);
    assembler.pop(AsmJit::ebp);
  }

  // Return
  assembler.ret(AsmJit::Imm(0x4));
#else
#error "[HadesMem] Unsupported architecture."
#endif
  
  DWORD_PTR const stub_size = assembler.getCodeSize();
  
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
  
  std::vector<RemoteFunctionRet> return_vals;
  for (std::size_t i = 0; i < addresses.size(); ++i)
  {
    DWORD_PTR const ret_val = Read<DWORD_PTR>(process, static_cast<DWORD_PTR*>(
      return_value_remote.GetBase()) + i);
    DWORD64 const ret_val_64 = Read<DWORD64>(process, static_cast<DWORD64*>(
      return_value_64_remote.GetBase()) + i);
    float const ret_val_float = Read<float>(process, static_cast<float*>(return_value_float_remote.GetBase()) + i);
    double const ret_val_double = Read<double>(process, static_cast<double*>(return_value_double_remote.GetBase()) + i);
    DWORD const error_code = Read<DWORD>(process, static_cast<DWORD*>(
      last_error_remote.GetBase()) + i);
    return_vals.push_back(RemoteFunctionRet(ret_val, ret_val_64, ret_val_float, ret_val_double, error_code));
  }
  
  return return_vals;
}

}
