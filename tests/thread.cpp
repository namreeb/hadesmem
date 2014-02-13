// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/thread.hpp>
#include <hadesmem/thread.hpp>

#include <locale>
#include <sstream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/thread_helpers.hpp>

template <typename Func> class ScopeExit
{
public:
  explicit ScopeExit(Func func) : func_(func)
  {
  }

  ScopeExit(ScopeExit const&) = delete;

  ScopeExit& operator=(ScopeExit const&) = delete;

  ~ScopeExit()
  {
    func_();
  }

private:
  Func func_;
};

DWORD WINAPI WaitFunc(LPVOID param)
{
  auto const quit_event_ = static_cast<hadesmem::detail::SmartHandle*>(param);
  BOOST_TEST_EQ(::WaitForSingleObject(quit_event_->GetHandle(), INFINITE),
                WAIT_OBJECT_0);
  return 0UL;
}

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

  hadesmem::detail::SmartHandle quit_event(
    ::CreateEvent(nullptr, TRUE, FALSE, nullptr));
  DWORD other_id = 0;
  hadesmem::detail::SmartHandle wait_thread(
    ::CreateThread(nullptr, 0, &WaitFunc, &quit_event, 0, &other_id));
  auto const cleanup_thread_func = [&]()
  {

    BOOST_TEST_NE(::SetEvent(quit_event.GetHandle()), 0);
    BOOST_TEST_EQ(::WaitForSingleObject(wait_thread.GetHandle(), INFINITE),
                  WAIT_OBJECT_0);
  };
  {
    ScopeExit<decltype(cleanup_thread_func)> cleanup_thread(
      cleanup_thread_func);

    hadesmem::Thread const other_thread(other_id);
    BOOST_TEST_EQ(hadesmem::SuspendThread(other_thread), 0UL);
    BOOST_TEST_EQ(hadesmem::SuspendThread(other_thread), 1UL);
    BOOST_TEST_EQ(hadesmem::ResumeThread(other_thread), 2UL);
    BOOST_TEST_EQ(hadesmem::ResumeThread(other_thread), 1UL);

    {
      hadesmem::SuspendedThread const suspend_thread(other_thread.GetId());

      CONTEXT const full_context =
        hadesmem::GetThreadContext(other_thread, CONTEXT_FULL);
      CONTEXT const all_context =
        hadesmem::GetThreadContext(other_thread, CONTEXT_ALL);
      hadesmem::SetThreadContext(other_thread, full_context);
      hadesmem::SetThreadContext(other_thread, all_context);
      BOOST_TEST_THROWS(hadesmem::GetThreadContext(thread, CONTEXT_FULL),
                        hadesmem::Error);
    }

    {
      hadesmem::SuspendedProcess const suspend_process(::GetCurrentProcessId());
    }
  }
}

int main()
{
  TestThisThread();
  return boost::report_errors();
}
