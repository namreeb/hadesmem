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
#include "hadesmem/detail/call_arg_visitor_x64.hpp"

namespace hadesmem
{

namespace detail
{

#if defined(_M_AMD64)

void GenerateCallCode64(AsmJit::X86Assembler* assembler, 
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

  auto const max_args_list = std::max_element(std::begin(args_full), 
    std::end(args_full), 
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

  assembler->sub(AsmJit::rsp, stack_offs);

  assembler->mov(AsmJit::rax, is_debugger_present);
  assembler->call(AsmJit::rax);

  assembler->test(AsmJit::rax, AsmJit::rax);
  assembler->jz(label_nodebug);

  assembler->mov(AsmJit::rax, debug_break);
  assembler->call(AsmJit::rax);

  assembler->bind(label_nodebug);

  assembler->mov(AsmJit::rcx, 0);
  assembler->mov(AsmJit::rax, set_last_error);
  assembler->call(AsmJit::rax);

  for (std::size_t i = 0; i < addresses.size(); ++i)
  {
    LPCVOID address = addresses[i];
    std::vector<CallArg> const& args = args_full[i];
    std::size_t const num_args = args.size();

    (void)call_convs;
    BOOST_ASSERT(call_convs[i] == CallConv::kDefault || 
      call_convs[i] == CallConv::kWinApi || 
      call_convs[i] == CallConv::kX64);

    detail::ArgVisitor64 arg_visitor(assembler, num_args);
    std::for_each(args.rbegin(), args.rend(), 
      [&] (CallArg const& arg)
    {
      arg.Visit(&arg_visitor);
    });

    assembler->mov(AsmJit::rax, reinterpret_cast<DWORD_PTR>(address));
    assembler->call(AsmJit::rax);

    assembler->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, return_value));
    assembler->mov(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::rax);

    assembler->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, return_value_32));
    assembler->mov(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::eax);

    assembler->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, return_value_64));
    assembler->mov(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::rax);

    assembler->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, return_value_float));
    assembler->movd(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::xmm0);

    assembler->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, return_value_double));
    assembler->movq(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::xmm0);

    assembler->mov(AsmJit::rax, get_last_error);
    assembler->call(AsmJit::rax);

    assembler->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(CallResultRemote) + 
      offsetof(CallResultRemote, last_error));
    assembler->mov(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::eax);
  }

  assembler->add(AsmJit::rsp, stack_offs);

  assembler->ret();
}

#endif // #if defined(_M_AMD64)

}

}
