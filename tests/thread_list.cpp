// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/thread_list.hpp>
#include <hadesmem/thread_list.hpp>

#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/thread.hpp>

DWORD WINAPI SleepWrapper(LPVOID /*param*/)
{
  ::Sleep(INFINITE);
  return 0UL;
}

void TestThreadList()
{
  using ThreadListIterCat =
    std::iterator_traits<hadesmem::ThreadList::iterator>::iterator_category;
  HADESMEM_DETAIL_STATIC_ASSERT(
    std::is_base_of<std::input_iterator_tag, ThreadListIterCat>::value);
  using ThreadListConstIterCat = std::iterator_traits<
    hadesmem::ThreadList::const_iterator>::iterator_category;
  HADESMEM_DETAIL_STATIC_ASSERT(
    std::is_base_of<std::input_iterator_tag, ThreadListConstIterCat>::value);

  {
    hadesmem::detail::SmartHandle wait_thread_1(
      ::CreateThread(nullptr, 0, &SleepWrapper, nullptr, 0, nullptr));
    hadesmem::detail::SmartHandle wait_thread_2(
      ::CreateThread(nullptr, 0, &SleepWrapper, nullptr, 0, nullptr));
  }

  hadesmem::ThreadList const thread_list_1(::GetCurrentProcessId());
  hadesmem::ThreadList thread_list_2(thread_list_1);
  hadesmem::ThreadList thread_list_3(std::move(thread_list_2));
  thread_list_2 = std::move(thread_list_3);
  BOOST_TEST(std::begin(thread_list_2) != std::end(thread_list_2));

  auto iter = std::begin(thread_list_1);
  BOOST_TEST(iter != std::end(thread_list_1));
  BOOST_TEST(++iter != std::end(thread_list_1));
  BOOST_TEST_NE(iter->GetId(), 0U);
  DWORD const second_id = iter->GetId();
  BOOST_TEST(++iter != std::end(thread_list_1));
  DWORD const third_id = iter->GetId();
  BOOST_TEST_NE(second_id, third_id);
}

void TestThreadListAlgorithm()
{
  hadesmem::ThreadList const thread_list_1(::GetCurrentProcessId());

  for (auto const& entry : thread_list_1)
  {
    BOOST_TEST_EQ(entry.GetUsage(), 0UL);
    BOOST_TEST_NE(entry.GetId(), 0UL);
    BOOST_TEST_EQ(entry.GetOwnerId(), ::GetCurrentProcessId());
    BOOST_TEST(entry.GetBasePriority() >= 0L);
    BOOST_TEST(entry.GetBasePriority() <= 31L);
    BOOST_TEST_EQ(entry.GetDeltaPriority(), 0L);
    BOOST_TEST_EQ(entry.GetFlags(), 0UL);
  }

  auto const this_iter = std::find_if(std::begin(thread_list_1),
                                      std::end(thread_list_1),
                                      [](hadesmem::ThreadEntry const& thread)
  { return thread.GetId() == ::GetCurrentThreadId(); });
  BOOST_TEST(this_iter != std::end(thread_list_1));
}

int main()
{
  TestThreadList();
  TestThreadListAlgorithm();
  return boost::report_errors();
}
