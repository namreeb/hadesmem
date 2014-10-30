// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <vector>
#include <set>
#include <sstream>

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
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("SuspendThread failed.")
                                    << ErrorCodeWinLast(last_error));
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
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("ResumeThread failed.")
                                    << ErrorCodeWinLast(last_error));
  }

  return suspend_count;
}

inline CONTEXT GetThreadContext(Thread const& thread, DWORD context_flags)
{
  if (::GetCurrentThreadId() == thread.GetId() &&
      context_flags != CONTEXT_DEBUG_REGISTERS)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("GetThreadContext called for current thread."));
  }

  CONTEXT context{};
  context.ContextFlags = context_flags;
  if (!::GetThreadContext(thread.GetHandle(), &context))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("GetThreadContext failed.")
                                    << ErrorCodeWinLast(last_error));
  }

  return context;
}

inline void SetThreadContext(Thread const& thread, CONTEXT const& context)
{
  if (!::SetThreadContext(thread.GetHandle(), &context))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("SetThreadContext failed.")
                                    << ErrorCodeWinLast(last_error));
  }
}

class SuspendedThread
{
public:
  explicit SuspendedThread(DWORD thread_id) : thread_(thread_id)
  {
    SuspendThread(thread_);
  }

  SuspendedThread(SuspendedThread const& other) = delete;

  SuspendedThread& operator=(SuspendedThread const& other) = delete;

  SuspendedThread(SuspendedThread&& other) HADESMEM_DETAIL_NOEXCEPT
    : thread_(std::move(other.thread_))
  {
  }

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

  DWORD GetId() const HADESMEM_DETAIL_NOEXCEPT
  {
    return thread_.GetId();
  }

  HANDLE GetHandle() const HADESMEM_DETAIL_NOEXCEPT
  {
    return thread_.GetHandle();
  }

private:
  void ResumeUnchecked()
  {
    try
    {
      Resume();
    }
    catch (...)
    {
      // WARNING: Thread is never resumed if ResumeThread fails...
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);
    }
  }

  Thread thread_;
};

class SuspendedProcess
{
public:
  explicit SuspendedProcess(DWORD pid, DWORD retries = 5)
  {
    // Multiple retries may be needed to plug a race condition
    // whereby after a thread snapshot is taken but before suspension
    // takes place, an existing thread launches a new thread which
    // would then be missed.
    std::set<DWORD> tids;
    bool need_retry = false;
    do
    {
      need_retry = false;

      ThreadList const threads(pid);
      for (auto const& thread_entry : threads)
      {
        DWORD const current_thread_id = ::GetCurrentThreadId();
        if (thread_entry.GetId() != current_thread_id)
        {
          auto const inserted = tids.insert(thread_entry.GetId());
          if (inserted.second)
          {
            need_retry = true;

            try
            {
              // Close potential race condition whereby after the snapshot is
              // taken the thread could terminate and have its TID reused in
              // a different process.
              Thread const thread(thread_entry.GetId());
              VerifyPid(thread, pid);

              // Should be safe (with the exception outlined above) to suspend
              // the thread now, because we have a handle to the thread open,
              // and we've verified it exists in the correct process.
              threads_.emplace_back(thread_entry.GetId());
            }
            catch (std::exception const& /*e*/)
            {
              tids.erase(inserted.first);
              continue;
            }
          }
        }
      }
    } while (need_retry && retries--);

    if (need_retry)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Failed to suspend all threads in process "
                               "(too many retries)."));
    }
  }

  SuspendedProcess(SuspendedProcess const& other) = delete;

  SuspendedProcess& operator=(SuspendedProcess const& other) = delete;

  SuspendedProcess(SuspendedProcess&& other) HADESMEM_DETAIL_NOEXCEPT
    : threads_(std::move(other.threads_))
  {
  }

  SuspendedProcess& operator=(SuspendedProcess&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    threads_ = std::move(other.threads_);

    return *this;
  }

private:
  void VerifyPid(Thread const& thread, DWORD pid) const
  {
    DWORD const tid_pid = ::GetProcessIdOfThread(thread.GetHandle());
    if (!tid_pid)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("GetProcessIdOfThread failed.")
                << ErrorCodeWinLast(last_error));
    }

    if (tid_pid != pid)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("PID verification failed."));
    }
  }
  std::vector<SuspendedThread> threads_;
};
}
