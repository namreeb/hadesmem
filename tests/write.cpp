// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/write.hpp"

#include <array>
#include <string>
#include <vector>

#define BOOST_TEST_MODULE write
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

BOOST_AUTO_TEST_CASE(write)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  struct TestPODType
  {
    int a;
    char* b;
    wchar_t c;
    long long d;
  };
  
  TestPODType test_pod_type = { 1, 0, L'a', 1234567812345678 };
  TestPODType test_pod_type_2 = { -1, 0, L'x', 9876543210 };
  Write(process, &test_pod_type, test_pod_type_2);
  BOOST_CHECK_EQUAL(std::memcmp(&test_pod_type, &test_pod_type_2, 
    sizeof(test_pod_type)), 0);
  
  std::string const test_string = "Narrow test string.";
  std::vector<char> test_string_buf(test_string.size() + 1);
  std::copy(begin(test_string), end(test_string), 
    test_string_buf.data());
  std::string const test_string_str(test_string_buf.data());
  BOOST_CHECK_EQUAL_COLLECTIONS(begin(test_string), end(test_string), 
    begin(test_string_str), end(test_string_str));
  auto const test_string_rev = std::string(test_string.rbegin(), 
    test_string.rend());
  WriteString(process, test_string_buf.data(), test_string_rev);
  auto const new_test_string_rev = std::string(test_string_buf.data());
  BOOST_CHECK_EQUAL_COLLECTIONS(new_test_string_rev.cbegin(), 
    new_test_string_rev.cend(), test_string_rev.cbegin(), test_string_rev.cend());
  
  std::array<int, 10> int_list = {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }};
  std::vector<int> int_list_rev(int_list.crbegin(), int_list.crend());
  WriteList(process, &int_list, int_list_rev);
  BOOST_CHECK_EQUAL_COLLECTIONS(int_list.cbegin(), int_list.cend(), 
    int_list_rev.cbegin(), int_list_rev.cend());
}
