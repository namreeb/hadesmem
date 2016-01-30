// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <functional>
#include <map>
#include <string>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/srw_lock.hpp>

#include "imgui.hpp"

// TODO: Fix the problem of registering/unregistring a callback from the context
// of a callback (deadlock in current implementation, iterator invalidation if
// recursive mutex is used).

namespace hadesmem
{
namespace cerberus
{
namespace detail
{
#if 0
// TODO: Move this somewhere more appropriate. Should probably be abstracted
// away from ImGui and moved to a generic overlay logging layer.
inline void LogWrapper(std::string const& s)
{
  auto& imgui = GetImguiInterface();
  imgui.Log(s);
}
#endif
}

template <typename Func> class Callbacks
{
public:
  using Callback = std::function<Func>;

  Callbacks() : srw_lock_(SRWLOCK_INIT)
  {
  }

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

  template <typename... Args> void Run(Args&&... args) const noexcept
  {
    hadesmem::detail::AcquireSRWLock lock(
      &srw_lock_, hadesmem::detail::SRWLockType::Shared);
    for (auto const& c : callbacks_)
    {
      try
      {
        c.second(std::forward<Args>(args)...);
      }
      catch (...)
      {
        // TODO: Re-enable this once we can implement it properly.
        // detail::LogWrapper(boost::current_exception_diagnostic_information());
        HADESMEM_DETAIL_TRACE_A(
          boost::current_exception_diagnostic_information());
      }
    }
  }

private:
  mutable SRWLOCK srw_lock_;
  std::size_t next_id_ = std::size_t{};
  std::map<std::size_t, Callback> callbacks_;
  std::map<std::size_t, Callback> pending_;
};
}
}
