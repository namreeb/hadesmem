// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/alloc.hpp>
#include <hadesmem/alloc.hpp>

#include <sstream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

void TestAlloc()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  PVOID address = Alloc(process, 0x1000);
  *static_cast<BYTE*>(address) = static_cast<BYTE>(0xFF);
  BOOST_TEST_EQ(*static_cast<BYTE*>(address), static_cast<BYTE>(0xFF));
  MEMORY_BASIC_INFORMATION mbi;
  ::ZeroMemory(&mbi, sizeof(mbi));
  BOOST_TEST(::VirtualQuery(address, &mbi, sizeof(mbi)));
  BOOST_TEST_EQ(mbi.BaseAddress, address);
  BOOST_TEST_EQ(mbi.RegionSize, 0x1000UL);
  BOOST_TEST_EQ(mbi.State, static_cast<DWORD>(MEM_COMMIT));
  BOOST_TEST_EQ(mbi.Protect, static_cast<DWORD>(PAGE_EXECUTE_READWRITE));
  BOOST_TEST_EQ(mbi.Type, static_cast<DWORD>(MEM_PRIVATE));
  Free(process, address);

  BOOST_TEST_THROWS(Alloc(process, 0), hadesmem::Error);
}

void TestAllocator()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::Allocator allocator_1(process, 0x1000);
  BOOST_TEST(allocator_1.GetBase());
  BOOST_TEST_EQ(allocator_1.GetSize(), 0x1000UL);

  hadesmem::Allocator allocator_2(std::move(allocator_1));
  BOOST_TEST(allocator_2.GetBase());
  BOOST_TEST_EQ(allocator_2.GetSize(), 0x1000UL);

  allocator_1 = std::move(allocator_2);
  BOOST_TEST(allocator_1.GetBase());
  BOOST_TEST_EQ(allocator_1.GetSize(), 0x1000UL);
  allocator_1.Free();

  hadesmem::Allocator allocator_3(process, 0x1000);
  hadesmem::Allocator allocator_4(process, 0x1000);
  BOOST_TEST_EQ(allocator_3, allocator_3);
  BOOST_TEST_NE(allocator_3, allocator_4);
  BOOST_TEST_NE(allocator_4, allocator_3);
  if (allocator_3 > allocator_4)
  {
    BOOST_TEST(allocator_3 > allocator_4);
    BOOST_TEST(allocator_3 >= allocator_4);
    BOOST_TEST(!(allocator_3 < allocator_4));
    BOOST_TEST(!(allocator_3 <= allocator_4));
  }
  else
  {
    BOOST_TEST(allocator_4 > allocator_3);
    BOOST_TEST(allocator_4 >= allocator_3);
    BOOST_TEST(!(allocator_4 < allocator_3));
    BOOST_TEST(!(allocator_4 <= allocator_3));
  }

  std::stringstream test_str_1;
  test_str_1.imbue(std::locale::classic());
  test_str_1 << allocator_3;
  std::stringstream test_str_2;
  test_str_2.imbue(std::locale::classic());
  test_str_2 << allocator_3.GetBase();
  BOOST_TEST_EQ(test_str_1.str(), test_str_2.str());
  std::stringstream test_str_3;
  test_str_3.imbue(std::locale::classic());
  test_str_3 << allocator_4.GetBase();
  BOOST_TEST_NE(test_str_1.str(), test_str_3.str());
}

int main()
{
  TestAlloc();
  TestAllocator();
  return boost::report_errors();
}
