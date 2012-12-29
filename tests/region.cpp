// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/region.hpp"

#include <locale>
#include <sstream>
#include <utility>

#define BOOST_TEST_MODULE region
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/config.hpp"
#include "hadesmem/process.hpp"

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

// Boost.Test causes the following warning under Clang:
// error: declaration requires a global constructor 
// [-Werror,-Wglobal-constructors]
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif // #if defined(HADESMEM_CLANG)

BOOST_TEST_DONT_PRINT_LOG_VALUE(std::wstring)

BOOST_AUTO_TEST_CASE(region)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  hadesmem::Region const first_region(&process, nullptr);
  hadesmem::Region first_region_2(first_region);
  hadesmem::Region first_region_3(std::move(first_region_2));
  first_region_2 = std::move(first_region_3);
  BOOST_CHECK_EQUAL(first_region, first_region_2);
  
  if (first_region.GetState() != MEM_FREE)
  {
    BOOST_CHECK_NE(first_region.GetBase(), static_cast<void*>(nullptr));
    BOOST_CHECK_NE(first_region.GetAllocBase(), static_cast<void*>(nullptr));
    BOOST_CHECK_NE(first_region.GetAllocProtect(), 0U);
    BOOST_CHECK_NE(first_region.GetType(), 0U);
  }
  
  first_region.GetProtect();
  
  BOOST_CHECK_NE(first_region.GetSize(), 0U);
  BOOST_CHECK_NE(first_region.GetState(), 0U);

  hadesmem::Region const first_region_other(&process, static_cast<PBYTE>(
    first_region.GetBase()) + 1);
  BOOST_CHECK_EQUAL(first_region.GetBase(), first_region_other.GetBase());
  BOOST_CHECK_EQUAL(first_region.GetAllocBase(), 
    first_region_other.GetAllocBase());
  BOOST_CHECK_EQUAL(first_region.GetAllocProtect(), 
    first_region_other.GetAllocProtect());
  BOOST_CHECK_EQUAL(first_region.GetSize(), first_region_other.GetSize());
  BOOST_CHECK_EQUAL(first_region.GetState(), first_region_other.GetState());
  BOOST_CHECK_EQUAL(first_region.GetProtect(), 
    first_region_other.GetProtect());
  BOOST_CHECK_EQUAL(first_region.GetType(), first_region_other.GetType());
  
  hadesmem::Region const second_region(&process, static_cast<char const* const>(
    first_region.GetBase()) + first_region.GetSize());
  BOOST_CHECK_NE(first_region, second_region);
  BOOST_CHECK_LT(first_region, second_region);
  BOOST_CHECK_LE(first_region, second_region);
  BOOST_CHECK_GT(second_region, first_region);
  BOOST_CHECK_GE(second_region, first_region);
  BOOST_CHECK(!(first_region > second_region));
  BOOST_CHECK(!(first_region >= second_region));
  BOOST_CHECK(!(second_region < first_region));
  BOOST_CHECK(!(second_region <= first_region));
  BOOST_CHECK_GE(first_region, first_region);
  BOOST_CHECK_LE(first_region, first_region);
  BOOST_CHECK_EQUAL(first_region, first_region);

  std::stringstream second_region_str_1;
  second_region_str_1.imbue(std::locale::classic());
  second_region_str_1 << second_region.GetBase();
  std::stringstream second_region_str_2;
  second_region_str_2.imbue(std::locale::classic());
  second_region_str_2 << second_region;
  BOOST_CHECK_EQUAL(second_region_str_1.str(), second_region_str_2.str());

  std::wstringstream second_region_str_3;
  second_region_str_3.imbue(std::locale::classic());
  second_region_str_3 << second_region.GetBase();
  std::wstringstream second_region_str_4;
  second_region_str_4.imbue(std::locale::classic());
  second_region_str_4 << second_region;
  BOOST_CHECK_EQUAL(second_region_str_3.str(), second_region_str_4.str());
}
