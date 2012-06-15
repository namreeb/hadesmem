// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/region_iterator.hpp"

#define BOOST_TEST_MODULE region_iterator
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/concept_check.hpp>
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/region.hpp"
#include "hadesmem/process.hpp"

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

BOOST_AUTO_TEST_CASE(module_iterator)
{
  BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::RegionIterator>));
  
  hadesmem::Process const process(::GetCurrentProcessId());
  
  auto iter = hadesmem::RegionIterator(process);
  hadesmem::Region const first_region(process, nullptr);
  BOOST_CHECK(iter != hadesmem::RegionIterator());
  BOOST_CHECK(*iter == first_region);
  hadesmem::Region const second_region(process, static_cast<char const* const>(
    first_region.GetBase()) + first_region.GetSize());
  BOOST_CHECK(++iter != hadesmem::RegionIterator());
  BOOST_CHECK(*iter == second_region);
  hadesmem::Region last(process, nullptr);
  do
  {
    last = *iter;
  } while (++iter != hadesmem::RegionIterator());
  // TODO: Compare our last region with the 'real' last region to ensure they 
  // match.
  
  std::for_each(hadesmem::RegionIterator(process), hadesmem::RegionIterator(), 
    [&] (hadesmem::Region const& region)
    {
      hadesmem::Region const other(process, region.GetBase());
      BOOST_CHECK(region == other);
      
      if (region.GetState() != MEM_FREE)
      {
        BOOST_CHECK(region.GetBase() != nullptr);
        BOOST_CHECK(region.GetAllocBase() != nullptr);
        BOOST_CHECK(region.GetAllocProtect() != 0);
        BOOST_CHECK(region.GetType() != 0);
      }
      
      region.GetProtect();
      
      BOOST_CHECK(region.GetSize() != 0);
      BOOST_CHECK(region.GetState() != 0);
    });
  
  HMODULE const user32_mod = GetModuleHandle(L"user32.dll");
  BOOST_CHECK(std::find_if(hadesmem::RegionIterator(process), 
    hadesmem::RegionIterator(), 
    [user32_mod] (hadesmem::Region const& region)
    {
      return region.GetBase() == user32_mod;
    }) != hadesmem::RegionIterator());
}
