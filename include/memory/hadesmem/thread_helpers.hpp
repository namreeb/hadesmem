// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <vector>
#include <sstream>
#include <algorithm>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/thread.hpp>
#include <hadesmem/thread_list.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/thread_entry.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/winapi.hpp>
#include <hadesmem/detail/smart_handle.hpp>

// TODO: Add tests for all thread helper APIs.

namespace hadesmem
{
  
inline DWORD SuspendThread(Thread const& thread)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Suspending thread with ID 0n%lu.", 
    thread.GetId());

  DWORD const suspend_count = ::SuspendThread(thread.GetHandle());
  if (suspend_count == static_cast<DWORD>(-1))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("SuspendThread failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return suspend_count;
}

inline DWORD ResumeThread(Thread const& thread)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Resuming thread with ID 0n%lu.", 
    thread.GetId());

  DWORD const suspend_count = ::ResumeThread(thread.GetHandle());
  if (suspend_count == static_cast<DWORD>(-1))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("ResumeThread failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return suspend_count;
}

inline CONTEXT GetThreadContext(Thread const& thread, DWORD context_flags)
{
  if (::GetCurrentThreadId() == thread.GetId())
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("GetThreadContext called for current thread."));
  }

  CONTEXT context;
  ZeroMemory(&context, sizeof(context));
  context.ContextFlags = context_flags;
  if (!::GetThreadContext(thread.GetHandle(), &context))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("GetThreadContext failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return context;
}

inline void SetThreadContext(Thread const& thread, CONTEXT const& context)
{
  if (!::SetThreadContext(thread.GetHandle(), &context))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("SetThreadContext failed.") << 
      ErrorCodeWinLast(last_error));
  }
}

class SuspendedThread
{
public:
  explicit SuspendedThread(DWORD thread_id)
    : thread_(thread_id)
  {
    SuspendThread(thread_);
  }

  SuspendedThread(SuspendedThread&& other) HADESMEM_DETAIL_NOEXCEPT
    : thread_(std::move(other.thread_))
  { }

  SuspendedThread& operator=(SuspendedThread&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    ResumeUnchecked();

    thread_ = std::move(other.thread_);

    return *this;
  }

  ~SuspendedThread() HADESMEM_DETAIL_NOEXCEPT
  {
    ResumeUnchecked();
  }

  void Resume()
  {
    if (thread_.GetHandle())
    {
      ResumeThread(thread_);
    }
  }

private:
  // Disable copying.
  SuspendedThread(SuspendedThread const& other);
  SuspendedThread& operator=(SuspendedThread const& other);

  void ResumeUnchecked()
  {
    try
    {
      Resume();
    }
    catch (std::exception const& e)
    {
      // WARNING: Thread is never resumed if ResumeThread fails...
      (void)e;
      HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());
      HADESMEM_DETAIL_ASSERT(false);
    }
  }

  Thread thread_;
};

// TODO: Do multiple passes to ensure no threads are missed. Currently this 
// code suffers from a race condition whereby new threads could be created 
// after we take the snapshot, so they would be missed.
// TODO: Fix this code for the case where a thread in the snapshot has 
// terminated by the time we get around to trying to suspend it. Should 
// errors just be swallowed? We could probably get away with that as long as 
// we fix the above todo and just do several attempts until all threads 
// have been successfully suspended (or back out if not).
class SuspendedProcess
{
public:
  explicit SuspendedProcess(DWORD pid)
  {
    ThreadList threads(pid);
    for (auto const& thread_entry : threads)
    {
      DWORD const current_thread_id = ::GetCurrentThreadId();
      if (thread_entry.GetId() != current_thread_id)
      {
        threads_.emplace_back(thread_entry.GetId());
      }
    }
  }

  SuspendedProcess(SuspendedProcess&& other) HADESMEM_DETAIL_NOEXCEPT
    : threads_(std::move(other.threads_))
  { }

  SuspendedProcess& operator=(SuspendedProcess&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    threads_ = std::move(other.threads_);

    return *this;
  }
  
private:
  // Disable copying.
  SuspendedProcess(SuspendedProcess const& other);
  SuspendedProcess& operator=(SuspendedProcess const& other);
  
  std::vector<SuspendedThread> threads_;
};

}
