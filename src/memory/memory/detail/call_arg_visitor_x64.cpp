// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/call_arg_visitor_x64.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <AsmJit/AsmJit.h>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/detail/static_assert.hpp"

namespace hadesmem
{

namespace detail
{

#if defined(_M_AMD64)

ArgVisitor64::ArgVisitor64(AsmJit::X86Assembler* assembler, 
  std::size_t num_args) BOOST_NOEXCEPT
  : assembler_(assembler), 
  num_args_(num_args), 
  cur_arg_(num_args)
{ }

void ArgVisitor64::operator()(DWORD32 arg) BOOST_NOEXCEPT
{
  return (*this)(static_cast<DWORD64>(arg));
}

void ArgVisitor64::operator()(DWORD64 arg) BOOST_NOEXCEPT
{
  switch (cur_arg_)
  {
  case 1:
    assembler_->mov(AsmJit::rcx, arg);
    break;
  case 2:
    assembler_->mov(AsmJit::rdx, arg);
    break;
  case 3:
    assembler_->mov(AsmJit::r8, arg);
    break;
  case 4:
    assembler_->mov(AsmJit::r9, arg);
    break;
  default:
    assembler_->mov(AsmJit::rax, arg);
    assembler_->mov(AsmJit::qword_ptr(AsmJit::rsp, cur_arg_ * 8 - 8), 
      AsmJit::rax);
    break;
  }

  --cur_arg_;
}

void ArgVisitor64::operator()(float arg) BOOST_NOEXCEPT
{
  HADESMEM_STATIC_ASSERT(sizeof(float) == sizeof(DWORD));

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

void ArgVisitor64::operator()(double arg) BOOST_NOEXCEPT
{
  HADESMEM_STATIC_ASSERT(sizeof(double) == sizeof(DWORD64));

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

#endif // #if defined(_M_AMD64)

}

}
