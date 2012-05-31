// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/memory.hpp"

#include <array>
#include <string>
#include <vector>

#define BOOST_TEST_MODULE memory
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif // #if defined(HADESMEM_GCC)
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

BOOST_AUTO_TEST_CASE(query_and_protect)
{
  hadesmem::Process const process(GetCurrentProcessId());
  
  HMODULE const this_mod = GetModuleHandle(nullptr);
  BOOST_CHECK(CanRead(process, this_mod));
  BOOST_CHECK(!CanWrite(process, this_mod));
  BOOST_CHECK(!CanExecute(process, this_mod));
  BOOST_CHECK(!IsGuard(process, this_mod));
  
  PVOID address = Alloc(process, 0x1000);
  BOOST_CHECK(CanRead(process, address));
  BOOST_CHECK(CanWrite(process, address));
  BOOST_CHECK(CanExecute(process, address));
  BOOST_CHECK(!IsGuard(process, address));
  BOOST_CHECK(Protect(process, address, PAGE_NOACCESS) == PAGE_EXECUTE_READWRITE);
  BOOST_CHECK(!CanRead(process, address));
  BOOST_CHECK(!CanWrite(process, address));
  BOOST_CHECK(!CanExecute(process, address));
  BOOST_CHECK(!IsGuard(process, address));
  BOOST_CHECK(Protect(process, address, PAGE_EXECUTE) == PAGE_NOACCESS);
  BOOST_CHECK(CanExecute(process, address));
  FlushInstructionCache(process, address, 0x1000);
  Free(process, address);
  
  LPVOID const invalid_address = reinterpret_cast<LPVOID>(
    static_cast<DWORD_PTR>(-1));
  BOOST_CHECK_THROW(Alloc(process, 0), hadesmem::HadesMemError);
  BOOST_CHECK_THROW(CanRead(process, invalid_address), hadesmem::HadesMemError);
  BOOST_CHECK_THROW(CanWrite(process, invalid_address), hadesmem::HadesMemError);
  BOOST_CHECK_THROW(CanExecute(process, invalid_address), hadesmem::HadesMemError);
  BOOST_CHECK_THROW(IsGuard(process, invalid_address), hadesmem::HadesMemError);
  BOOST_CHECK_THROW(Protect(process, invalid_address, PAGE_EXECUTE_READWRITE), hadesmem::HadesMemError);
  BOOST_CHECK_THROW(FlushInstructionCache(process, invalid_address, 1), hadesmem::HadesMemError);
  BOOST_CHECK_THROW(Free(process, invalid_address), hadesmem::HadesMemError);
}

BOOST_AUTO_TEST_CASE(ReadWriteTest)
{
  hadesmem::Process const process(GetCurrentProcessId());
  
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
  TestPODType test_pod_type_2 = { -1, 0, L'x', 9876543210 };
  Write(process, &test_pod_type, test_pod_type_2);
  BOOST_CHECK_EQUAL(std::memcmp(&test_pod_type, &test_pod_type_2, 
    sizeof(test_pod_type)), 0);
  
  char const* const test_string = "Narrow test string.";
  char* const test_string_real = const_cast<char*>(test_string);
  auto const new_test_string = hadesmem::ReadString<std::string>(process, 
    test_string_real);
  BOOST_CHECK_EQUAL(new_test_string, test_string);
  auto const test_string_str = std::string(test_string);
  auto const test_string_rev = std::string(test_string_str.rbegin(), 
    test_string_str.rend());
  WriteString(process, test_string_real, test_string_rev);
  auto const new_test_string_rev = hadesmem::ReadString<std::string>(process, 
    test_string_real);
  BOOST_CHECK_EQUAL_COLLECTIONS(new_test_string_rev.cbegin(), 
    new_test_string_rev.cend(), test_string_rev.cbegin(), test_string_rev.cend());
  
  std::array<int, 10> int_list = {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }};
  std::vector<int> int_list_read = hadesmem::ReadList<std::vector<int>>(process, 
    &int_list, 10);
  BOOST_CHECK_EQUAL_COLLECTIONS(int_list.cbegin(), int_list.cend(), 
    int_list_read.cbegin(), int_list_read.cend());
  std::vector<int> int_list_rev(int_list_read.crbegin(), int_list_read.crend());
  WriteList(process, &int_list, int_list_rev);
  BOOST_CHECK_EQUAL_COLLECTIONS(int_list.cbegin(), int_list.cend(), 
    int_list_rev.cbegin(), int_list_rev.cend());
}
