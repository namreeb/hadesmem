// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/region.hpp"

#define BOOST_TEST_MODULE region
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

BOOST_AUTO_TEST_CASE(region)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  hadesmem::Region const first_region(process, nullptr);
  
  if (first_region.GetState() != MEM_FREE)
  {
    BOOST_CHECK(first_region.GetBase() != nullptr);
    BOOST_CHECK(first_region.GetAllocBase() != nullptr);
    BOOST_CHECK(first_region.GetAllocProtect() != 0);
    BOOST_CHECK(first_region.GetType() != 0);
  }
  
  first_region.GetProtect();
  
  BOOST_CHECK(first_region.GetSize() != 0);
  BOOST_CHECK(first_region.GetState() != 0);
  
  hadesmem::Region const second_region(process, static_cast<char const* const>(
    first_region.GetBase()) + first_region.GetSize());
  BOOST_CHECK(first_region < second_region);
  BOOST_CHECK(first_region <= second_region);
  BOOST_CHECK(second_region > first_region);
  BOOST_CHECK(second_region >= first_region);
  BOOST_CHECK(first_region >= first_region);
  BOOST_CHECK(first_region <= first_region);
  BOOST_CHECK(first_region == first_region);
  BOOST_CHECK(first_region != second_region);
}
