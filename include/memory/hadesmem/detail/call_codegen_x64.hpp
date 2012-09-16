// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <vector>

#include <windows.h>

namespace AsmJit
{

struct X86Assembler;

}

namespace hadesmem
{

class CallArg;
enum class CallConv;

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
  PVOID return_values_remote);

#endif // #if defined(_M_AMD64)

}

}