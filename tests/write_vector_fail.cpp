// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/write.hpp>

#define BOOST_TEST_MODULE write_vector_fail
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

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

struct non_vector_type
{ };

BOOST_AUTO_TEST_CASE(write_vector_fail)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  WriteVector(process, nullptr, non_vector_type());

  // Need Boost.Test to think it needs to log something to suppress a warning 
  // that would cause a compile failure and mask a test failure if the above 
  // code actually did compile.
  BOOST_CHECK_EQUAL(1, 1);
}
