// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/call.hpp"

#define BOOST_TEST_MODULE call
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

int call_test_target(int a, int b)
{
  return a * b;
}

BOOST_AUTO_TEST_CASE(call)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  int const arg0 = 5;
  int const arg1 = 10;
  int result = hadesmem::Call<int (*)(int, int)>(process, hadesmem::CallConv::kDefault, reinterpret_cast<void*>(&call_test_target), arg0, arg1);
  BOOST_CHECK_EQUAL(result, arg0 * arg1);
}
