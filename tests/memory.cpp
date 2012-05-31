// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/memory.hpp"

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
