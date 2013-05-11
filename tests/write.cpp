// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/write.hpp>

#include <array>
#include <string>
#include <vector>

#define BOOST_TEST_MODULE write
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/winapi.hpp>
#include <hadesmem/detail/initialize.hpp>

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

BOOST_AUTO_TEST_CASE(initialize)
{
  hadesmem::detail::InitializeAll();
}

BOOST_AUTO_TEST_CASE(write_pod)
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
  hadesmem::Write(process, &test_pod_type, test_pod_type_2);
  BOOST_CHECK_EQUAL(std::memcmp(&test_pod_type, &test_pod_type_2, 
    sizeof(test_pod_type)), 0);

  TestPODType test_pod_type_3 = { 1, 0, L'a', 1234567812345678 };
  char test_pod_raw[sizeof(TestPODType)] = {};
  std::copy(reinterpret_cast<unsigned char*>(&test_pod_type_2), 
    reinterpret_cast<unsigned char*>(&test_pod_type_2) + sizeof(TestPODType), 
    &test_pod_raw[0]);
  BOOST_CHECK_NE(std::memcmp(&test_pod_type_3, &test_pod_type_2, 
    sizeof(test_pod_type_3)), 0);
  hadesmem::Write(process, &test_pod_type_3, test_pod_raw);
  BOOST_CHECK_EQUAL(std::memcmp(&test_pod_type_3, &test_pod_type_2, 
    sizeof(test_pod_type_3)), 0);

  TestPODType test_pod_type_4 = { 1, 0, L'a', 1234567812345678 };
  hadesmem::Write(process, &test_pod_type_4, test_pod_raw, sizeof(TestPODType));
  BOOST_CHECK_EQUAL(std::memcmp(&test_pod_type_4, &test_pod_type_2, 
    sizeof(test_pod_type_4)), 0);

  TestPODType test_pod_type_5 = { 1, 0, L'a', 1234567812345678 };
  char const* const test_pod_raw_beg = test_pod_raw;
  char const* const test_pod_raw_end = test_pod_raw + sizeof(TestPODType);
  hadesmem::Write(process, &test_pod_type_5, test_pod_raw_beg, test_pod_raw_end);
  BOOST_CHECK_EQUAL(std::memcmp(&test_pod_type_5, &test_pod_type_2, 
    sizeof(test_pod_type_5)), 0);

  int int_arr_1[] = { 1, 2, 3, 4, 5 };
  int new_inner_1[] = { 9, 8 };
  hadesmem::Write(process, int_arr_1, new_inner_1, sizeof(new_inner_1) / sizeof(int));
  int expected_arr_1[] = { 9, 8, 3, 4, 5 };
  BOOST_CHECK_EQUAL(std::memcmp(&int_arr_1[0], &expected_arr_1[0], 
    sizeof(int_arr_1)), 0);

  int int_arr_2[] = { 1, 2, 3, 4, 5 };
  int new_inner_2[] = { 9, 8 };
  hadesmem::Write(process, int_arr_2, new_inner_2, new_inner_2 + 
    (sizeof(new_inner_2) / sizeof(int)));
  int expected_arr_2[] = { 9, 8, 3, 4, 5 };
  BOOST_CHECK_EQUAL(std::memcmp(&int_arr_2[0], &expected_arr_2[0], 
    sizeof(int_arr_2)), 0);

  PVOID const noaccess_page = VirtualAlloc(nullptr, sizeof(void*), 
    MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
  BOOST_REQUIRE(noaccess_page != nullptr);
  hadesmem::Write(process, noaccess_page, static_cast<void*>(nullptr));

  PVOID const guard_page = VirtualAlloc(nullptr, sizeof(void*), 
    MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE | PAGE_GUARD);
  BOOST_REQUIRE(guard_page != nullptr);
  BOOST_CHECK_THROW(hadesmem::Write(process, guard_page, 
    static_cast<void*>(nullptr)), hadesmem::Error);

  PVOID const readonly_page = VirtualAlloc(nullptr, sizeof(void*), 
    MEM_RESERVE | MEM_COMMIT, PAGE_READONLY);
  BOOST_REQUIRE(readonly_page != nullptr);
  hadesmem::Write<void*>(process, readonly_page, static_cast<void*>(nullptr));
}

BOOST_AUTO_TEST_CASE(write_string)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  std::string const test_string = "Narrow test string.";
  std::vector<char> test_string_buf(test_string.size() + 1);
  std::copy(std::begin(test_string), std::end(test_string), 
    test_string_buf.data());
  std::string const test_string_str(test_string_buf.data());
  BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(test_string), std::end(test_string), 
    std::begin(test_string_str), std::end(test_string_str));
  auto const test_string_rev = std::string(test_string.rbegin(), 
    test_string.rend());
  hadesmem::WriteString(process, test_string_buf.data(), test_string_rev);
  auto const new_test_string_rev = std::string(test_string_buf.data());
  BOOST_CHECK_EQUAL_COLLECTIONS(new_test_string_rev.cbegin(), 
    new_test_string_rev.cend(), test_string_rev.cbegin(), 
    test_string_rev.cend());

  char const test_array_string[] = "TestArrayString";
  char test_array[sizeof(test_array_string)] = { };
  hadesmem::WriteString(process, test_array, test_array_string);
  BOOST_CHECK_EQUAL(std::memcmp(test_array_string, test_array, 
    sizeof(test_array_string)), 0);

  char const test_ptr_string_data[] = "TestPtrString";
  char const* const test_ptr_string = test_ptr_string_data;
  char test_array_2[sizeof(test_ptr_string_data)] = { };
  hadesmem::WriteString(process, test_array_2, test_ptr_string);
  BOOST_CHECK_EQUAL(std::memcmp(test_ptr_string_data, test_array_2, 
    sizeof(test_ptr_string_data)), 0);

  struct Foo
  {
    char str[7];
  };
  Foo foo = { "FooBar" };
  char const* const test_replacement_beg = "Bar";
  hadesmem::WriteString(process, &foo.str, test_replacement_beg, 
    test_replacement_beg + 3);
  BOOST_CHECK_EQUAL(std::string(foo.str), std::string("BarBar"));
}

BOOST_AUTO_TEST_CASE(write_vector)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  std::array<int, 10> int_list = {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }};
  std::vector<int> int_list_rev(int_list.crbegin(), int_list.crend());
  hadesmem::WriteVector(process, &int_list, int_list_rev);
  BOOST_CHECK_EQUAL_COLLECTIONS(int_list.cbegin(), int_list.cend(), 
    int_list_rev.cbegin(), int_list_rev.cend());
}

BOOST_AUTO_TEST_CASE(write_cross_region)
{
  SYSTEM_INFO const sys_info = hadesmem::detail::GetSystemInfo();
  DWORD const page_size = sys_info.dwPageSize;
  
  PVOID const address = VirtualAlloc(nullptr, page_size * 2, 
    MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
  BOOST_REQUIRE(address != 0);
  DWORD old_protect = 0;
  // Disable warning false positive.
  // warning C6387: 'address' could be '0':  this does not adhere to the 
  // specification for the function 'VirtualProtect'.
#ifdef HADESMEM_MSVC
#pragma warning(push)
#pragma warning(disable: 6387)
#endif
  BOOST_CHECK(VirtualProtect(address, page_size, PAGE_READONLY, 
    &old_protect) != 0);
#ifdef HADESMEM_MSVC
#pragma warning(pop)
#endif
  hadesmem::Process const process(::GetCurrentProcessId());
  std::vector<char> buf(page_size * 2, 'h');
  hadesmem::WriteVector(process, address, buf);

  BOOST_CHECK(VirtualProtect(address, page_size * 2, PAGE_READWRITE, 
    &old_protect) != 0);

  char const* const beg = static_cast<char const* const>(address);
  char const* const end = beg + (page_size * 2);
  std::vector<char> check(beg, end);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.cbegin(), buf.cend(), check.cbegin(), 
    check.cend());
}
