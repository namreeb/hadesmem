// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/process_iterator.hpp"

#define BOOST_TEST_MODULE process_iterator
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/concept_check.hpp>
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

BOOST_AUTO_TEST_CASE(process_iterator)
{
  BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ProcessIterator>));
  
  auto iter = hadesmem::ProcessIterator(0);
  BOOST_CHECK(iter != hadesmem::ProcessIterator());
  BOOST_CHECK(++iter != hadesmem::ProcessIterator());
  BOOST_CHECK(iter->id != 0);
  DWORD const second_id = iter->id;
  BOOST_CHECK(++iter != hadesmem::ProcessIterator());
  DWORD const third_id = iter->id;
  BOOST_CHECK(second_id != third_id);
  
  std::for_each(hadesmem::ProcessIterator(0), 
    hadesmem::ProcessIterator(), 
    [] (hadesmem::ProcessEntry const& entry)
    {
      BOOST_CHECK(!entry.name.empty());
    });
  
  auto this_iter = std::find_if(hadesmem::ProcessIterator(0), 
    hadesmem::ProcessIterator(), 
    [] (hadesmem::ProcessEntry const& entry)
    {
      return entry.id == ::GetCurrentProcessId();
    });
  BOOST_CHECK(this_iter != hadesmem::ProcessIterator());
}
