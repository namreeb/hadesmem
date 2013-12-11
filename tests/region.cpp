// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/region.hpp>

#include <locale>
#include <sstream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

void TestRegion()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::Region const first_region(process, nullptr);
  hadesmem::Region first_region_2(first_region);
  hadesmem::Region first_region_3(std::move(first_region_2));
  first_region_2 = std::move(first_region_3);
  BOOST_TEST_EQ(first_region, first_region_2);

  if (first_region.GetState() != MEM_FREE)
  {
    BOOST_TEST_NE(first_region.GetBase(), static_cast<void*>(nullptr));
    BOOST_TEST_NE(first_region.GetAllocBase(), static_cast<void*>(nullptr));
    BOOST_TEST_NE(first_region.GetAllocProtect(), 0U);
    BOOST_TEST_NE(first_region.GetType(), 0U);
  }

  first_region.GetProtect();

  BOOST_TEST_NE(first_region.GetSize(), 0U);
  BOOST_TEST_NE(first_region.GetState(), 0U);

  hadesmem::Region const first_region_other(
    process, static_cast<PBYTE>(first_region.GetBase()) + 1);
  BOOST_TEST_EQ(first_region.GetBase(), first_region_other.GetBase());
  BOOST_TEST_EQ(first_region.GetAllocBase(), first_region_other.GetAllocBase());
  BOOST_TEST_EQ(first_region.GetAllocProtect(),
                first_region_other.GetAllocProtect());
  BOOST_TEST_EQ(first_region.GetSize(), first_region_other.GetSize());
  BOOST_TEST_EQ(first_region.GetState(), first_region_other.GetState());
  BOOST_TEST_EQ(first_region.GetProtect(), first_region_other.GetProtect());
  BOOST_TEST_EQ(first_region.GetType(), first_region_other.GetType());

  hadesmem::Region const second_region(
    process,
    static_cast<char const* const>(first_region.GetBase()) +
      first_region.GetSize());
  BOOST_TEST_NE(first_region, second_region);
  BOOST_TEST(first_region < second_region);
  BOOST_TEST(first_region <= second_region);
  BOOST_TEST(second_region > first_region);
  BOOST_TEST(second_region >= first_region);
  BOOST_TEST(!(first_region > second_region));
  BOOST_TEST(!(first_region >= second_region));
  BOOST_TEST(!(second_region < first_region));
  BOOST_TEST(!(second_region <= first_region));
  BOOST_TEST(first_region >= first_region);
  BOOST_TEST(first_region <= first_region);
  BOOST_TEST_EQ(first_region, first_region);

  std::stringstream second_region_str_1;
  second_region_str_1.imbue(std::locale::classic());
  second_region_str_1 << second_region.GetBase();
  std::stringstream second_region_str_2;
  second_region_str_2.imbue(std::locale::classic());
  second_region_str_2 << second_region;
  BOOST_TEST(second_region_str_1.str() == second_region_str_2.str());

  std::wstringstream second_region_str_3;
  second_region_str_3.imbue(std::locale::classic());
  second_region_str_3 << second_region.GetBase();
  std::wstringstream second_region_str_4;
  second_region_str_4.imbue(std::locale::classic());
  second_region_str_4 << second_region;
  BOOST_TEST(second_region_str_3.str() == second_region_str_4.str());
}

int main()
{
  TestRegion();
  return boost::report_errors();
}
