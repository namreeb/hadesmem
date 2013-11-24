// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/thread.hpp>

#include <locale>
#include <sstream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/thread.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/thread_helpers.hpp>

void TestThisThread()
{
    hadesmem::Thread thread(::GetCurrentThreadId());
    BOOST_TEST_EQ(thread, thread);
    hadesmem::Thread thread_new(::GetCurrentThreadId());
    BOOST_TEST_EQ(thread, thread_new);
    hadesmem::Thread const thread_moved(std::move(thread_new));
    hadesmem::Thread thread_copy(thread);
    BOOST_TEST_EQ(thread_copy, thread);
    thread = thread_copy;
    BOOST_TEST_EQ(thread, thread_copy);
    thread_new = std::move(thread_copy);
    BOOST_TEST_EQ(thread, thread_new);
    BOOST_TEST(thread >= thread);
    BOOST_TEST(thread <= thread);
    BOOST_TEST_EQ(thread.GetId(), ::GetCurrentThreadId());
    std::stringstream thread_str_1;
    thread_str_1.imbue(std::locale::classic());
    thread_str_1 << thread;
    DWORD thread_id_1;
    thread_str_1 >> thread_id_1;
    BOOST_TEST_EQ(thread.GetId(), thread_id_1);
    std::wstringstream thread_str_2;
    thread_str_2.imbue(std::locale::classic());
    thread_str_2 << thread;
    DWORD thread_id_2;
    thread_str_2 >> thread_id_2;
    BOOST_TEST_EQ(thread.GetId(), thread_id_2);

    hadesmem::detail::SmartHandle quit_event(::CreateEvent(
        nullptr,
        TRUE,
        FALSE,
        nullptr));
    boost::thread other(
        [&]()
    {
        BOOST_TEST_EQ(::WaitForSingleObject(
            quit_event.GetHandle(),
            INFINITE),
            WAIT_OBJECT_0);
    });
    try
    {
        DWORD const other_id = ::GetThreadId(other.native_handle());
        hadesmem::Thread const other_thread(other_id);
        BOOST_TEST_EQ(hadesmem::SuspendThread(other_thread), 0UL);
        BOOST_TEST_EQ(hadesmem::SuspendThread(other_thread), 1UL);
        BOOST_TEST_EQ(hadesmem::ResumeThread(other_thread), 2UL);
        BOOST_TEST_EQ(hadesmem::ResumeThread(other_thread), 1UL);

        {
            hadesmem::SuspendedThread const suspend_thread(
                other_thread.GetId());

            CONTEXT const full_context = hadesmem::GetThreadContext(
                other_thread, 
                CONTEXT_FULL);
            CONTEXT const all_context = hadesmem::GetThreadContext(
                other_thread, 
                CONTEXT_ALL);
            hadesmem::SetThreadContext(other_thread, full_context);
            hadesmem::SetThreadContext(other_thread, all_context);
            BOOST_TEST_THROWS(hadesmem::GetThreadContext(
                thread, 
                CONTEXT_FULL), hadesmem::Error);
        }

        {
            hadesmem::SuspendedProcess const suspend_process(
                ::GetCurrentProcessId());
        }

        BOOST_TEST_NE(::SetEvent(quit_event.GetHandle()), 0);
        other.join();
    }
    catch (std::exception const& /*e*/)
    {
        BOOST_TEST_NE(::SetEvent(quit_event.GetHandle()), 0);
        other.join();
        throw;
    }
}

int main()
{
    TestThisThread();
    return boost::report_errors();
}
