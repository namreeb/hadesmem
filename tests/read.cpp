// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/read.hpp"

#include <array>
#include <string>
#include <vector>

#define BOOST_TEST_MODULE read
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

BOOST_AUTO_TEST_CASE(read)
{
  hadesmem::Process const process(::GetCurrentProcessId(), 
    hadesmem::ProcessAccess::kFull);
  
  struct TestPODType
  {
    int a;
    char* b;
    wchar_t c;
    long long d;
  };
  
  TestPODType test_pod_type = { 1, 0, L'a', 1234567812345678 };
  auto new_test_pod_type = hadesmem::Read<TestPODType>(process, &test_pod_type);
  BOOST_CHECK_EQUAL(std::memcmp(&test_pod_type, &new_test_pod_type, 
    sizeof(test_pod_type)), 0);
  
  std::string test_string = "Narrow test string.";
  char* const test_string_real = &test_string[0];
  auto const new_test_string = hadesmem::ReadString<std::string>(process, 
    test_string_real);
  BOOST_CHECK_EQUAL(new_test_string, test_string);
  
  std::array<int, 10> int_list = {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }};
  std::vector<int> int_list_read = hadesmem::ReadList<std::vector<int>>(process, 
    &int_list, 10);
  BOOST_CHECK_EQUAL_COLLECTIONS(int_list.cbegin(), int_list.cend(), 
    int_list_read.cbegin(), int_list_read.cend());
}
