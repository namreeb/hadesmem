// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <thread>

#include <hadesmem/detail/assert.hpp>

namespace hadesmem
{
namespace detail
{
class ThreadPool
{
public:
  ThreadPool(std::size_t pool_size, std::size_t queue_factor)
    : running_(true), queue_factor_(queue_factor)
  {
    for (std::size_t i = 0; i < pool_size; ++i)
    {
      threads_.emplace_back(std::bind(&ThreadPool::Main, this));
    }
  }

  ~ThreadPool()
  {
    {
      std::unique_lock<std::mutex> queued_lock(mutex_);
      running_ = false;
      queued_condition_.notify_all();
    }

    try
    {
      for (auto& t : threads_)
      {
        t.join();
      }
    }
    catch (...)
    {
    }
  }

  template <typename Task> bool QueueTask(Task const& task)
  {
    std::unique_lock<std::mutex> lock(mutex_);

    if (tasks_.size() >= threads_.size() * queue_factor_)
    {
      return false;
    }

    tasks_.emplace(task);
    queued_condition_.notify_one();

    return true;
  }

  void WaitForSlot()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    while (tasks_.size() >= threads_.size() * queue_factor_ && running_)
    {
      consumed_condition_.wait(lock);
    }
  }

  void WaitForEmpty()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    while (tasks_.size() && running_)
    {
      consumed_condition_.wait(lock);
    }
  }

  void Stop()
  {
    running_ = false;
  }

private:
  void Main()
  {
    while (running_)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      while (tasks_.empty() && running_)
      {
        queued_condition_.wait(lock);
      }

      if (!running_)
      {
        break;
      }

      {
        std::function<void()> task = tasks_.front();
        tasks_.pop();
        consumed_condition_.notify_one();

        lock.unlock();

        try
        {
          task();
        }
        catch (...)
        {
          // Tasks should be doing their own EH.
          HADESMEM_DETAIL_ASSERT(false);
        }
      }

      lock.lock();
    }
  }

  bool running_;
  std::size_t queue_factor_;
  std::queue<std::function<void()>> tasks_;
  std::vector<std::thread> threads_;
  std::mutex mutex_;
  std::condition_variable queued_condition_;
  std::condition_variable consumed_condition_;
};
}
}
