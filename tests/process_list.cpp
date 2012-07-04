// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/process_list.hpp"

#define BOOST_TEST_MODULE process_list
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/concept_check.hpp>
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/process_iterator.hpp"

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

BOOST_TEST_DONT_PRINT_LOG_VALUE(hadesmem::ProcessList::iterator)

BOOST_AUTO_TEST_CASE(process_list)
{
  BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ProcessList::
    iterator>));
  BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ProcessList::
    const_iterator>));
  
  using std::begin;
  using std::end;
  
  hadesmem::ProcessList const process_list_1;
  hadesmem::ProcessList process_list_2(process_list_1);
  hadesmem::ProcessList process_list_3(std::move(process_list_2));
  process_list_2 = std::move(process_list_3);
  BOOST_CHECK_NE(begin(process_list_2), end(process_list_2));
  
  auto iter = begin(process_list_1);
  BOOST_CHECK_NE(iter, end(process_list_1));
  BOOST_CHECK_NE(++iter, end(process_list_1));
  BOOST_CHECK_NE(iter->id, 0U);
  DWORD const second_id = iter->id;
  BOOST_CHECK_NE(++iter, end(process_list_1));
  DWORD const third_id = iter->id;
  BOOST_CHECK_NE(second_id, third_id);
}

BOOST_AUTO_TEST_CASE(process_list_algorithm)
{
  using std::begin;
  using std::end;
  
  hadesmem::ProcessList const process_list_1;
  
  std::for_each(begin(process_list_1), end(process_list_1), 
    [] (hadesmem::ProcessEntry const& entry)
    {
      BOOST_CHECK(!entry.name.empty());
    });
  
  auto const this_iter = std::find_if(begin(process_list_1), 
    end(process_list_1), 
    [] (hadesmem::ProcessEntry const& entry)
    {
      return entry.id == ::GetCurrentProcessId();
    });
  BOOST_CHECK_NE(this_iter, end(process_list_1));
}
