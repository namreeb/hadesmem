// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/thread.hpp>

#include <locale>
#include <sstream>
#include <utility>

#define BOOST_TEST_MODULE process
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/thread.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/thread_helpers.hpp>
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

BOOST_AUTO_TEST_CASE(initialize)
{
  hadesmem::detail::InitializeAll();
}

BOOST_AUTO_TEST_CASE(this_thread)
{
  hadesmem::Thread thread(::GetCurrentThreadId());
  BOOST_CHECK_EQUAL(thread, thread);
  hadesmem::Thread thread_new(::GetCurrentThreadId());
  BOOST_CHECK_EQUAL(thread, thread_new);
  hadesmem::Thread const thread_moved(std::move(thread_new));
  hadesmem::Thread thread_copy(thread);
  BOOST_CHECK_EQUAL(thread_copy, thread);
  thread = thread_copy;
  BOOST_CHECK_EQUAL(thread, thread_copy);
  thread_new = std::move(thread_copy);
  BOOST_CHECK_EQUAL(thread, thread_new);
  BOOST_CHECK_GE(thread, thread);
  BOOST_CHECK_LE(thread, thread);
  BOOST_CHECK_EQUAL(thread.GetId(), ::GetCurrentThreadId());
  std::stringstream thread_str_1;
  thread_str_1.imbue(std::locale::classic());
  thread_str_1 << thread;
  DWORD thread_id_1;
  thread_str_1 >> thread_id_1;
  BOOST_CHECK_EQUAL(thread.GetId(), thread_id_1);
  std::wstringstream thread_str_2;
  thread_str_2.imbue(std::locale::classic());
  thread_str_2 << thread;
  DWORD thread_id_2;
  thread_str_2 >> thread_id_2;
  BOOST_CHECK_EQUAL(thread.GetId(), thread_id_2);

  hadesmem::detail::SmartHandle quit_event(::CreateEvent(
    nullptr, 
    TRUE, 
    FALSE, 
    nullptr));
  boost::thread other(
    [&]()
    {
      BOOST_CHECK_EQUAL(::WaitForSingleObject(
        quit_event.GetHandle(), 
        INFINITE), 
        WAIT_OBJECT_0);
    });
  try
  {
    DWORD const other_id = ::GetThreadId(other.native_handle());
    hadesmem::Thread const other_thread(other_id);
    BOOST_CHECK_EQUAL(hadesmem::SuspendThread(other_thread), 0UL);
    BOOST_CHECK_EQUAL(hadesmem::SuspendThread(other_thread), 1UL);
    BOOST_CHECK_EQUAL(hadesmem::ResumeThread(other_thread), 2UL);
    BOOST_CHECK_EQUAL(hadesmem::ResumeThread(other_thread), 1UL);

    {
      hadesmem::SuspendedThread const suspend_thread(
        other_thread.GetId());

      CONTEXT const full_context = hadesmem::GetThreadContext(
        other_thread, CONTEXT_FULL);
      CONTEXT const all_context = hadesmem::GetThreadContext(
        other_thread, CONTEXT_ALL);
      hadesmem::SetThreadContext(other_thread, full_context);
      hadesmem::SetThreadContext(other_thread, all_context);
      BOOST_CHECK_THROW(hadesmem::GetThreadContext(thread, CONTEXT_FULL), 
        hadesmem::Error);
    }

    {
      hadesmem::SuspendedProcess const suspend_process(
        ::GetCurrentProcessId());
    }
    BOOST_CHECK_NE(::SetEvent(quit_event.GetHandle()), 0);
    other.join();
  }
  catch (std::exception const& /*e*/)
  {
    BOOST_CHECK_NE(::SetEvent(quit_event.GetHandle()), 0);
    other.join();
    throw;
  }
}
