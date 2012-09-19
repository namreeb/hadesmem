// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/call_codegen.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <AsmJit/AsmJit.h>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/call.hpp"
#include "hadesmem/alloc.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/call_codegen_x86.hpp"
#include "hadesmem/detail/call_codegen_x64.hpp"

namespace hadesmem
{

namespace detail
{
  
Allocator GenerateCallCode(Process const& process, 
  std::vector<LPCVOID> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full, 
  PVOID return_values_remote)
{
  Module const kernel32(&process, L"kernel32.dll");
  DWORD_PTR const get_last_error = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "GetLastError"));
  DWORD_PTR const set_last_error = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "SetLastError"));
  DWORD_PTR const is_debugger_present = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "IsDebuggerPresent"));
  DWORD_PTR const debug_break = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "DebugBreak"));

  AsmJit::X86Assembler assembler;
  
#if defined(_M_AMD64)
  detail::GenerateCallCode64(&assembler, addresses, call_convs, args_full, 
    get_last_error, set_last_error, is_debugger_present, debug_break, 
    return_values_remote);
#elif defined(_M_IX86)
  detail::GenerateCallCode32(&assembler, addresses, call_convs, args_full, 
    get_last_error, set_last_error, is_debugger_present, debug_break, 
    return_values_remote);
#else
#error "[HadesMem] Unsupported architecture."
#endif
  
  DWORD_PTR const stub_size = assembler.getCodeSize();
  
  Allocator stub_mem_remote(&process, stub_size);
  
  std::vector<BYTE> code_real(stub_size);
  assembler.relocCode(code_real.data(), reinterpret_cast<DWORD_PTR>(
    stub_mem_remote.GetBase()));
  
  WriteVector(process, stub_mem_remote.GetBase(), code_real);

  return stub_mem_remote;
}

}

}
