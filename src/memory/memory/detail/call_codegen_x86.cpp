// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/call_codegen_x64.hpp"

#include <algorithm>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <asmjit/asmjit.h>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/call.hpp"
#include "hadesmem/detail/call_remote_data.hpp"
#include "hadesmem/detail/call_arg_visitor_x86.hpp"

namespace hadesmem
{

namespace detail
{

#if defined(_M_IX86)
  
void GenerateCallCode32(AsmJit::X86Assembler* assembler, 
  std::vector<LPCVOID> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full, 
  DWORD_PTR get_last_error, 
  DWORD_PTR set_last_error, 
  DWORD_PTR is_debugger_present, 
  DWORD_PTR debug_break, 
  PVOID return_values_remote)
{
  AsmJit::Label label_nodebug(assembler->newLabel());

  assembler->push(AsmJit::ebp);
  assembler->mov(AsmJit::ebp, AsmJit::esp);

  assembler->mov(AsmJit::eax, is_debugger_present);
  assembler->call(AsmJit::eax);

  assembler->test(AsmJit::eax, AsmJit::eax);
  assembler->jz(label_nodebug);

  assembler->mov(AsmJit::eax, debug_break);
  assembler->call(AsmJit::eax);

  assembler->bind(label_nodebug);

  assembler->push(AsmJit::imm(0x0));
  assembler->mov(AsmJit::eax, set_last_error);
  assembler->call(AsmJit::eax);

  for (std::size_t i = 0; i < addresses.size(); ++i)
  {
    LPCVOID address = addresses[i];
    CallConv call_conv = call_convs[i];
    std::vector<CallArg> const& args = args_full[i];
    std::size_t const num_args = args.size();

    detail::ArgVisitor32 arg_visitor(assembler, num_args, call_conv);
    std::for_each(args.rbegin(), args.rend(), 
      [&] (CallArg const& arg)
    {
      arg.Visit(&arg_visitor);
    });

    assembler->mov(AsmJit::eax, reinterpret_cast<DWORD_PTR>(address));
    assembler->call(AsmJit::eax);

    assembler->mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, return_value));
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);

    assembler->mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, return_value_32));
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);

    assembler->mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, return_value_64));
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx, 4), AsmJit::edx);

    assembler->mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, return_value_float));
    assembler->fst(AsmJit::dword_ptr(AsmJit::ecx));

    assembler->mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, return_value_double));
    assembler->fst(AsmJit::qword_ptr(AsmJit::ecx));

    assembler->mov(AsmJit::eax, get_last_error);
    assembler->call(AsmJit::eax);

    assembler->mov(AsmJit::ecx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, last_error));
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);

    if (call_conv == CallConv::kDefault || call_conv == CallConv::kCdecl)
    {
      assembler->add(AsmJit::esp, AsmJit::imm(num_args * sizeof(PVOID)));
    }
  }

  assembler->mov(AsmJit::esp, AsmJit::ebp);
  assembler->pop(AsmJit::ebp);

  assembler->ret(AsmJit::imm(0x4));
}

#endif // #if defined(_M_IX86)

}

}
