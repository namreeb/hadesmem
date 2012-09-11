// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <cstddef>

#include <boost/config.hpp>

#include <windows.h>

namespace AsmJit
{

struct X86Assembler;

}

namespace hadesmem
{

namespace detail
{

#if defined(_M_AMD64)

class ArgVisitor64
{
public:
  ArgVisitor64(AsmJit::X86Assembler* assembler, std::size_t num_args) 
    BOOST_NOEXCEPT;

  void operator()(DWORD32 arg) BOOST_NOEXCEPT;

  void operator()(DWORD64 arg) BOOST_NOEXCEPT;

  void operator()(float arg) BOOST_NOEXCEPT;

  void operator()(double arg) BOOST_NOEXCEPT;

private:
  AsmJit::X86Assembler* assembler_;
  std::size_t num_args_;
  std::size_t cur_arg_;
};

#endif // #if defined(_M_AMD64)

}

}
