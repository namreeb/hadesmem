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

// Boost.Test causes the following warning under Clang:
// error: declaration requires a global constructor 
// [-Werror,-Wglobal-constructors]
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif // #if defined(HADESMEM_CLANG)

// TODO: Improve tests by doing checks both before and after writes.
// TODO: Don't read/write data on the stack.
// TODO: Test reads on guard pages, noaccess pages, etc.

// TODO: Provide the appropriate stream operator overload to allow this (also 
// ensuring the streams used by Boost.Test are imbued with a UTF-8 locale).
BOOST_TEST_DONT_PRINT_LOG_VALUE(std::wstring)

BOOST_AUTO_TEST_CASE(read_pod)
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
  auto new_test_pod_type = hadesmem::Read<TestPODType>(process, 
    &test_pod_type);
  BOOST_CHECK_EQUAL(std::memcmp(&test_pod_type, &new_test_pod_type, 
    sizeof(test_pod_type)), 0);

  auto const new_test_array = 
    hadesmem::Read<std::array<char, sizeof(TestPODType)>>(process, 
    &test_pod_type);
  BOOST_CHECK_EQUAL(std::memcmp(&test_pod_type, &new_test_array[0], 
    sizeof(test_pod_type)), 0);

  auto const new_test_array_2 = 
    hadesmem::Read<char, sizeof(TestPODType)>(process, 
    &test_pod_type);
  BOOST_CHECK_EQUAL(std::memcmp(&test_pod_type, &new_test_array_2[0], 
    sizeof(test_pod_type)), 0);
}

BOOST_AUTO_TEST_CASE(read_string)
{
  hadesmem::Process const process(::GetCurrentProcessId());

  std::string test_string = "Narrow test string.";
  char* const test_string_real = &test_string[0];
  auto const new_test_string = hadesmem::ReadString<char>(process, 
    test_string_real);
  BOOST_CHECK_EQUAL(new_test_string, test_string);

  std::wstring wide_test_string = L"Narrow test string.";
  wchar_t* const wide_test_string_real = &wide_test_string[0];
  auto const wide_new_test_string = hadesmem::ReadString<wchar_t>(process, 
    wide_test_string_real);
  BOOST_CHECK_EQUAL(wide_new_test_string, wide_test_string);

  std::string test_string_2 = "Narrow test string.";
  char* const test_string_real_2 = &test_string_2[0];
  auto const new_test_string_2 = hadesmem::ReadString<char>(process, 
    test_string_real_2, 1);
  BOOST_CHECK_EQUAL(new_test_string_2, test_string_2);
}

BOOST_AUTO_TEST_CASE(read_vector)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  std::array<int, 10> int_list = {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }};
  std::vector<int> int_list_read = hadesmem::ReadVector<int>(
    process, &int_list, 10);
  BOOST_CHECK_EQUAL_COLLECTIONS(int_list.cbegin(), int_list.cend(), 
    int_list_read.cbegin(), int_list_read.cend());
}
