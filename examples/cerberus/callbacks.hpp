// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <functional>
#include <map>
#include <mutex>

#include <hadesmem/detail/srw_lock.hpp>

namespace hadesmem
{

namespace cerberus
{

template <typename Func> class Callbacks
{
public:
  using Callback = std::function<Func>;

  std::size_t Register(Callback const& callback)
  {
    hadesmem::detail::AcquireSRWLock lock(
      &srw_lock_, hadesmem::detail::SRWLockType::Exclusive);
    auto const cur_id = next_id_++;
    HADESMEM_DETAIL_ASSERT(next_id_ > cur_id);
    callbacks_[cur_id] = callback;
    return cur_id;
  }

  void Unregister(std::size_t id)
  {
    hadesmem::detail::AcquireSRWLock lock(
      &srw_lock_, hadesmem::detail::SRWLockType::Exclusive);
    auto const num_removed = callbacks_.erase(id);
    HADESMEM_DETAIL_ASSERT(num_removed == 1);
    (void)num_removed;
  }

  template <typename... Args> void Run(Args&&... args) const
  {
    hadesmem::detail::AcquireSRWLock lock(
      &srw_lock_, hadesmem::detail::SRWLockType::Shared);
    for (auto const& callback : callbacks_)
    {
      callback.second(std::forward<Args&&>(args)...);
    }
  }

private:
  mutable SRWLOCK srw_lock_ = SRWLOCK_INIT;
  std::size_t next_id_ = std::size_t{};
  std::map<std::size_t, Callback> callbacks_;
};
}
}
