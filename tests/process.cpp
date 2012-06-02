// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/process.hpp"

#define BOOST_TEST_MODULE process
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

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

BOOST_AUTO_TEST_CASE(this_process)
{
  hadesmem::Process process(GetCurrentProcessId());
  BOOST_CHECK_EQUAL(process.GetId(), GetCurrentProcessId());
  std::string const path(hadesmem::GetPath(process));
  BOOST_CHECK(!path.empty());
  BOOST_CHECK(boost::filesystem::exists(path));
  BOOL is_wow64_real = FALSE;
  BOOST_CHECK(IsWow64Process(GetCurrentProcess(), &is_wow64_real));
  BOOST_CHECK_EQUAL(hadesmem::IsWoW64(process), is_wow64_real != FALSE);
  BOOST_CHECK_NO_THROW(process.Cleanup());
}
