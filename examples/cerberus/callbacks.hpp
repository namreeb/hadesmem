// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <functional>
#include <map>
#include <mutex>

template <typename Func> class Callbacks
{
public:
  using Callback = std::function<Func>;

  std::size_t Register(Callback const& callback)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto const cur_id = next_id_++;
    HADESMEM_DETAIL_ASSERT(next_id_ > cur_id);
    callbacks_[cur_id] = callback;
    return cur_id;
  }

  void Unregister(std::size_t id)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.erase(id);
  }

  template <typename... Args> void Run(Args&&... args) const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto const& callback : callbacks_)
    {
      callback.second(std::forward<Args&&>(args)...);
    }
  }

private:
  mutable std::mutex mutex_;
  std::size_t next_id_ = std::size_t{};
  std::map<std::size_t, Callback> callbacks_;
};
