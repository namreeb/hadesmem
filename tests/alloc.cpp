// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/alloc.hpp"

#define BOOST_TEST_MODULE alloc
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
  
  LPVOID const invalid_address = reinterpret_cast<LPVOID>(
    static_cast<DWORD_PTR>(-1));
  BOOST_CHECK_THROW(Alloc(process, 0), hadesmem::HadesMemError);
  BOOST_CHECK_THROW(Free(process, invalid_address), hadesmem::HadesMemError);
}

// TODO: Test Allocator stream overloads.

BOOST_AUTO_TEST_CASE(allocator)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  hadesmem::Allocator allocator_1(&process, 0x1000);
  BOOST_CHECK(allocator_1.GetBase());
  BOOST_CHECK_EQUAL(allocator_1.GetSize(), static_cast<SIZE_T>(0x1000));
  
  hadesmem::Allocator allocator_2(std::move(allocator_1));
  BOOST_CHECK(allocator_2.GetBase());
  BOOST_CHECK_EQUAL(allocator_2.GetSize(), static_cast<SIZE_T>(0x1000));
  
  BOOST_CHECK(!allocator_1.GetBase());
  BOOST_CHECK_EQUAL(allocator_1.GetSize(), static_cast<SIZE_T>(0));
  BOOST_CHECK_NO_THROW(allocator_1.Free());
  
  allocator_1 = std::move(allocator_2);
  BOOST_CHECK(allocator_1.GetBase());
  BOOST_CHECK_EQUAL(allocator_1.GetSize(), static_cast<SIZE_T>(0x1000));
  
  BOOST_CHECK(!allocator_2.GetBase());
  BOOST_CHECK_EQUAL(allocator_2.GetSize(), static_cast<SIZE_T>(0));
  BOOST_CHECK_NO_THROW(allocator_2.Free());
  
  BOOST_CHECK_NO_THROW(allocator_1.Free());

  hadesmem::Allocator allocator_3(&process, 0x1000);
  hadesmem::Allocator allocator_4(&process, 0x1000);
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
}
