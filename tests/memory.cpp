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
  
  TestPODType MyTestPODType = { 1, 0, L'a', 1234567812345678 };
  auto MyNewTestPODType = hadesmem::Read<TestPODType>(process, &MyTestPODType);
  BOOST_CHECK_EQUAL(std::memcmp(&MyTestPODType, &MyNewTestPODType, 
    sizeof(TestPODType)), 0);
  TestPODType MyTestPODType2 = { -1, 0, L'x', 9876543210 };
  hadesmem::Write(process, &MyTestPODType, MyTestPODType2);
  BOOST_CHECK_EQUAL(std::memcmp(&MyTestPODType, &MyTestPODType2, 
    sizeof(TestPODType)), 0);
  
  char const* const pTestStringA = "Narrow test string.";
  char* const pTestStringAReal = const_cast<char*>(pTestStringA);
  auto const NewTestStringA = hadesmem::ReadString<std::string>(process, 
    pTestStringAReal);
  BOOST_CHECK_EQUAL(NewTestStringA, pTestStringA);
  auto const TestStringAStr = std::string(pTestStringA);
  auto const TestStringARev = std::string(TestStringAStr.rbegin(), 
    TestStringAStr.rend());
  hadesmem::WriteString(process, pTestStringAReal, TestStringARev);
  auto const NewTestStringARev = hadesmem::ReadString<std::string>(process, 
    pTestStringAReal);
  BOOST_CHECK_EQUAL_COLLECTIONS(NewTestStringARev.cbegin(), 
    NewTestStringARev.cend(), TestStringARev.cbegin(), TestStringARev.cend());
  
  wchar_t const* const pTestStringW = L"Wide test string.";
  wchar_t* const pTestStringWReal = const_cast<wchar_t*>(pTestStringW);
  auto const NewTestStringW = hadesmem::ReadString<std::wstring>(process, 
    pTestStringWReal);
  // Note: BOOST_CHECK_EQUAL does not support wide strings it seems
  BOOST_CHECK(NewTestStringW == pTestStringW);
  auto const TestStringWStr = std::wstring(pTestStringW);
  auto const TestStringWRev = std::wstring(TestStringWStr.rbegin(), 
    TestStringWStr.rend());
  hadesmem::WriteString(process, pTestStringWReal, TestStringWRev);
  auto const NewTestStringWRev = hadesmem::ReadString<std::wstring>(process, 
    pTestStringWReal);
  // Note: BOOST_CHECK_EQUAL does not support wide strings it seems
  BOOST_CHECK_EQUAL_COLLECTIONS(NewTestStringWRev.cbegin(), 
    NewTestStringWRev.cend(), TestStringWRev.cbegin(), TestStringWRev.cend());
  
  std::array<int, 10> IntList = {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }};
  std::vector<int> IntListRead = hadesmem::ReadList<std::vector<int>>(process, 
    &IntList, 10);
  BOOST_CHECK_EQUAL_COLLECTIONS(IntList.cbegin(), IntList.cend(), 
    IntListRead.cbegin(), IntListRead.cend());
  std::vector<int> IntListRev(IntListRead.crbegin(), IntListRead.crend());
  hadesmem::WriteList(process, &IntList, IntListRev);
  BOOST_CHECK_EQUAL_COLLECTIONS(IntList.cbegin(), IntList.cend(), 
    IntListRev.cbegin(), IntListRev.cend());
}
