// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/thread_list.hpp>

#include <utility>

#define BOOST_TEST_MODULE thread_list
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/thread.hpp>
#include <boost/concept_check.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/thread.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/detail/initialize.hpp>

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

BOOST_TEST_DONT_PRINT_LOG_VALUE(hadesmem::ThreadList::iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(hadesmem::ThreadList::const_iterator)
  
BOOST_AUTO_TEST_CASE(initialize)
{
  hadesmem::detail::InitializeAll();
}

BOOST_AUTO_TEST_CASE(thread_list)
{
  BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ThreadList::
    iterator>));
  BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ThreadList::
    const_iterator>));

  boost::thread second_thread([]() { Sleep(INFINITE); });
  second_thread.detach();
  boost::thread third_thread([]() { Sleep(INFINITE); });
  third_thread.detach();
  
  hadesmem::ThreadList const thread_list_1(::GetCurrentProcessId());
  hadesmem::ThreadList thread_list_2(thread_list_1);
  hadesmem::ThreadList thread_list_3(std::move(thread_list_2));
  thread_list_2 = std::move(thread_list_3);
  BOOST_CHECK_NE(std::begin(thread_list_2), std::end(thread_list_2));
  
  auto iter = std::begin(thread_list_1);
  BOOST_CHECK_NE(iter, std::end(thread_list_1));
  BOOST_CHECK_NE(++iter, std::end(thread_list_1));
  BOOST_CHECK_NE(iter->GetId(), 0U);
  DWORD const second_id = iter->GetId();
  BOOST_CHECK_NE(++iter, std::end(thread_list_1));
  DWORD const third_id = iter->GetId();
  BOOST_CHECK_NE(second_id, third_id);
}

BOOST_AUTO_TEST_CASE(thread_list_algorithm)
{
  hadesmem::ThreadList const thread_list_1(::GetCurrentProcessId());
  
  for (auto const& entry : thread_list_1)
  {
    BOOST_CHECK_EQUAL(entry.GetUsage(), 0UL);
    BOOST_CHECK_NE(entry.GetId(), 0UL);
    BOOST_CHECK_EQUAL(entry.GetOwnerId(), ::GetCurrentProcessId());
    BOOST_CHECK_GE(entry.GetBasePriority(), 0L);
    BOOST_CHECK_LE(entry.GetBasePriority(), 31L);
    BOOST_CHECK_EQUAL(entry.GetDeltaPriority(), 0L);
    BOOST_CHECK_EQUAL(entry.GetFlags(), 0UL);
  }
  
  auto const this_iter = std::find_if(
    std::begin(thread_list_1), 
    std::end(thread_list_1), 
    [] (hadesmem::ThreadEntry const& thread)
    {
      return thread.GetId() == ::GetCurrentThreadId();
    });
  BOOST_CHECK_NE(this_iter, std::end(thread_list_1));
}
