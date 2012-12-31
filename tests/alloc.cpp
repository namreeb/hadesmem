// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/alloc.hpp"

#include <sstream>
#include <utility>

#define BOOST_TEST_MODULE alloc
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

BOOST_AUTO_TEST_CASE(alloc)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  PVOID address = Alloc(process, 0x1000);
  *static_cast<BYTE*>(address) = static_cast<BYTE>(0xFF);
  BOOST_CHECK_EQUAL(*static_cast<BYTE*>(address), static_cast<BYTE>(0xFF));
  MEMORY_BASIC_INFORMATION mbi;
  ::ZeroMemory(&mbi, sizeof(mbi));
  BOOST_REQUIRE(::VirtualQuery(address, &mbi, sizeof(mbi)));
  BOOST_CHECK_EQUAL(mbi.BaseAddress, address);
  BOOST_CHECK_EQUAL(mbi.RegionSize, static_cast<SIZE_T>(0x1000));
  BOOST_CHECK_EQUAL(mbi.State, static_cast<DWORD>(MEM_COMMIT));
  BOOST_CHECK_EQUAL(mbi.Protect, static_cast<DWORD>(PAGE_EXECUTE_READWRITE));
  BOOST_CHECK_EQUAL(mbi.Type, static_cast<DWORD>(MEM_PRIVATE));
  BOOST_CHECK_NO_THROW(Free(process, address));
  
  auto const invalid_address = reinterpret_cast<LPVOID>(
    static_cast<DWORD_PTR>(-1));
  BOOST_CHECK_THROW(Alloc(process, 0), hadesmem::Error);
  BOOST_CHECK_THROW(Free(process, invalid_address), hadesmem::Error);
}

BOOST_AUTO_TEST_CASE(allocator)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  hadesmem::Allocator allocator_1(process, 0x1000);
  BOOST_CHECK(allocator_1.GetBase());
  BOOST_CHECK_EQUAL(allocator_1.GetSize(), static_cast<SIZE_T>(0x1000));
  
  hadesmem::Allocator allocator_2(std::move(allocator_1));
  BOOST_CHECK(allocator_2.GetBase());
  BOOST_CHECK_EQUAL(allocator_2.GetSize(), static_cast<SIZE_T>(0x1000));
  
  allocator_1 = std::move(allocator_2);
  BOOST_CHECK(allocator_1.GetBase());
  BOOST_CHECK_EQUAL(allocator_1.GetSize(), static_cast<SIZE_T>(0x1000));
  
  BOOST_CHECK_NO_THROW(allocator_1.Free());

  hadesmem::Allocator allocator_3(process, 0x1000);
  hadesmem::Allocator allocator_4(process, 0x1000);
  BOOST_CHECK_EQUAL(allocator_3, allocator_3);
  BOOST_CHECK_NE(allocator_3, allocator_4);
  BOOST_CHECK_NE(allocator_4, allocator_3);
  if (allocator_3 > allocator_4)
  {
    BOOST_CHECK_GT(allocator_3, allocator_4);
    BOOST_CHECK_GE(allocator_3, allocator_4);
    BOOST_CHECK(!(allocator_3 < allocator_4));
    BOOST_CHECK(!(allocator_3 <= allocator_4));
  }
  else
  {
    BOOST_CHECK_GT(allocator_4, allocator_3);
    BOOST_CHECK_GE(allocator_4, allocator_3);
    BOOST_CHECK(!(allocator_4 < allocator_3));
    BOOST_CHECK(!(allocator_4 <= allocator_3));
  }

  std::stringstream test_str_1;
  test_str_1.imbue(std::locale::classic());
  test_str_1 << allocator_3;
  std::stringstream test_str_2;
  test_str_2 << allocator_3.GetBase();
  BOOST_CHECK_EQUAL(test_str_1.str(), test_str_2.str());
  std::stringstream test_str_3;
  test_str_3.imbue(std::locale::classic());
  test_str_3 << allocator_4.GetBase();
  BOOST_CHECK_NE(test_str_1.str(), test_str_3.str());
}
