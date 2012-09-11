// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/call_arg_visitor_x86.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <AsmJit/AsmJit.h>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/call.hpp"
#include "hadesmem/detail/static_assert.hpp"

namespace hadesmem
{

namespace detail
{

#if defined(_M_IX86)

ArgVisitor32::ArgVisitor32(AsmJit::X86Assembler* assembler, 
  std::size_t num_args, CallConv call_conv) BOOST_NOEXCEPT
  : assembler_(assembler), 
  num_args_(num_args), 
  cur_arg_(num_args), 
  call_conv_(call_conv)
{ }

void ArgVisitor32::operator()(DWORD32 arg) BOOST_NOEXCEPT
{
  switch (cur_arg_)
  {
  case 1:
    switch (call_conv_)
    {
    case CallConv::kThisCall:
    case CallConv::kFastCall:
      assembler_->mov(AsmJit::ecx, arg);
      break;
    default:
      assembler_->mov(AsmJit::eax, arg);
      assembler_->push(AsmJit::eax);
      break;
    }
    break;
  case 2:
    switch (call_conv_)
    {
    case CallConv::kFastCall:
      assembler_->mov(AsmJit::edx, arg);
      break;
    default:
      assembler_->mov(AsmJit::eax, arg);
      assembler_->push(AsmJit::eax);
      break;
    }
    break;
  default:
    assembler_->mov(AsmJit::eax, arg);
    assembler_->push(AsmJit::eax);
    break;
  }

  --cur_arg_;
}

void ArgVisitor32::operator()(DWORD64 arg) BOOST_NOEXCEPT
{
  // TODO: Test __fastcall with a 64-bit arg to ensure this is correct.

  assembler_->mov(AsmJit::eax, static_cast<DWORD>((arg >> 32) & 
    0xFFFFFFFF));
  assembler_->push(AsmJit::eax);

  assembler_->mov(AsmJit::eax, static_cast<DWORD>(arg));
  assembler_->push(AsmJit::eax);

  --cur_arg_;
}

void ArgVisitor32::operator()(float arg) BOOST_NOEXCEPT
{
  HADESMEM_STATIC_ASSERT(sizeof(float) == sizeof(DWORD));

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

void ArgVisitor32::operator()(double arg) BOOST_NOEXCEPT
{
  HADESMEM_STATIC_ASSERT(sizeof(double) == sizeof(DWORD64));

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

#endif // #if defined(_M_IX86)

}

}
