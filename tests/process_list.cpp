// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/process_list.hpp"

#include <utility>

#define BOOST_TEST_MODULE process_list
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/concept_check.hpp>
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/config.hpp"
#include "hadesmem/process_entry.hpp"

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

BOOST_TEST_DONT_PRINT_LOG_VALUE(hadesmem::ProcessList::iterator)

BOOST_AUTO_TEST_CASE(process_list)
{
  BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ProcessList::
    iterator>));
  BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ProcessList::
    const_iterator>));
  
  hadesmem::ProcessList const process_list_1;
  hadesmem::ProcessList process_list_2(process_list_1);
  hadesmem::ProcessList process_list_3(std::move(process_list_2));
  process_list_2 = std::move(process_list_3);
  BOOST_CHECK_NE(std::begin(process_list_2), std::end(process_list_2));
  
  auto iter = std::begin(process_list_1);
  BOOST_CHECK_NE(iter, std::end(process_list_1));
  BOOST_CHECK_NE(++iter, std::end(process_list_1));
  BOOST_CHECK_NE(iter->GetId(), 0U);
  DWORD const second_id = iter->GetId();
  BOOST_CHECK_NE(++iter, std::end(process_list_1));
  DWORD const third_id = iter->GetId();
  BOOST_CHECK_NE(second_id, third_id);
}

BOOST_AUTO_TEST_CASE(process_list_algorithm)
{
  hadesmem::ProcessList const process_list_1;
  
  for (auto const& entry : process_list_1)
  {
    BOOST_CHECK(!entry.GetName().empty());
  }
  
  auto const this_iter = std::find_if(std::begin(process_list_1), 
    std::end(process_list_1), 
    [] (hadesmem::ProcessEntry const& entry)
    {
      return entry.GetId() == ::GetCurrentProcessId();
    });
  BOOST_CHECK_NE(this_iter, std::end(process_list_1));
}
