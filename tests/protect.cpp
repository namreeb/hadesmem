// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/protect.hpp>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

void TestQuery()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  HMODULE const this_mod = GetModuleHandle(nullptr);
  BOOST_TEST(CanRead(process, this_mod));
  BOOST_TEST(!CanWrite(process, this_mod));
  BOOST_TEST(!CanExecute(process, this_mod));
  BOOST_TEST(!IsGuard(process, this_mod));
  BOOST_TEST(!IsNoCache(process, this_mod));
  BOOST_TEST(!IsWriteCombine(process, this_mod));
  BOOST_TEST(!IsBadProtect(process, this_mod));
}

void TestProtect()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  PVOID address = VirtualAlloc(
    nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  BOOST_TEST(address);
  BOOST_TEST(CanRead(process, address));
  BOOST_TEST(CanWrite(process, address));
  BOOST_TEST(CanExecute(process, address));
  BOOST_TEST(!IsGuard(process, address));
  BOOST_TEST_EQ(Protect(process, address, PAGE_NOACCESS),
                static_cast<DWORD>(PAGE_EXECUTE_READWRITE));
  BOOST_TEST(!CanRead(process, address));
  BOOST_TEST(!CanWrite(process, address));
  BOOST_TEST(!CanExecute(process, address));
  BOOST_TEST(!IsGuard(process, address));
  BOOST_TEST(!IsNoCache(process, address));
  BOOST_TEST(!IsWriteCombine(process, address));
  BOOST_TEST(!IsBadProtect(process, address));
  BOOST_TEST_EQ(Protect(process, address, PAGE_EXECUTE),
                static_cast<DWORD>(PAGE_NOACCESS));
  BOOST_TEST(CanExecute(process, address));
}

void QueryAndProtectInvalid()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  LPVOID const invalid_address =
    reinterpret_cast<LPVOID>(static_cast<DWORD_PTR>(-1));
  BOOST_TEST_THROWS(CanRead(process, invalid_address), hadesmem::Error);
  BOOST_TEST_THROWS(CanWrite(process, invalid_address), hadesmem::Error);
  BOOST_TEST_THROWS(CanExecute(process, invalid_address), hadesmem::Error);
  BOOST_TEST_THROWS(IsGuard(process, invalid_address), hadesmem::Error);
  BOOST_TEST_THROWS(IsNoCache(process, invalid_address), hadesmem::Error);
  BOOST_TEST_THROWS(IsWriteCombine(process, invalid_address), hadesmem::Error);
  BOOST_TEST_THROWS(IsBadProtect(process, invalid_address), hadesmem::Error);
  BOOST_TEST_THROWS(Protect(process, invalid_address, PAGE_EXECUTE_READWRITE),
                    hadesmem::Error);
}

int main()
{
  TestQuery();
  TestProtect();
  QueryAndProtectInvalid();
  return boost::report_errors();
}
