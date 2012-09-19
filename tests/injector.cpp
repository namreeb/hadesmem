// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/injector.hpp"

#define BOOST_TEST_MODULE alloc
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

BOOST_AUTO_TEST_CASE(injector)
{
  hadesmem::Process const process(::GetCurrentProcessId());

  HMODULE const kernel32_mod = GetModuleHandle(L"kernel32.dll");
  BOOST_REQUIRE(kernel32_mod != nullptr);

  // 'Inject' Kernel32 with path resolution disabled and ensure that the 
  // module handle matches the one retrieved via GetModuleHandle earlier.
  HMODULE const kernel32_mod_new = hadesmem::InjectDll(process, 
    L"kernel32.dll", hadesmem::InjectFlags::None);
  BOOST_CHECK_EQUAL(kernel32_mod, kernel32_mod_new);

  // Free kernel32.dll in remote process.
  hadesmem::FreeDll(process, kernel32_mod_new);

  // Todo: Test path resolution.

  // Todo: Test CreateAndInject API.

}
