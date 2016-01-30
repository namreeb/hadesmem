// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <functional>
#include <map>
#include <string>

#include "callbacks.hpp"

// TODO: Implement this properly.

namespace hadesmem
{
namespace cerberus
{
template <typename ContextT> class Events
{
public:
  using EventCallbacks = Callbacks<void(ContextT* context)>;
  using EventCallbackFn = typename EventCallbacks::Callback;
  using NamedEventCallbackFn =
    std::function<void(std::wstring const& name, ContextT* context)>;

  std::size_t Register(std::wstring const& name, EventCallbackFn const& callback)
  {
    return events_[name].Register(callback);
  }

  void Unregister(std::wstring const& name, std::size_t id)
  {
    auto const iter = events_.find(name);
    if (iter != std::end(events_))
    {
      iter->second.Unregister(id);
    }
    else
    {
      HADESMEM_DETAIL_ASSERT(false);
    }
  }

  void Fire(std::wstring const& name, ContextT* context)
  {
    // Call this first because it's generally only useful for debugging
    // purposes, at which point we'd want to be called before any 'real'
    // callbacks so we can do any necessary logging (or whatever else we're
    // doing) for what event is being fired.
    if (named_callback_)
    {
      named_callback_(name, context);
    }

    auto const iter = events_.find(name);
    if (iter != std::end(events_))
    {
      iter->second.Run(context);
    }
  }

  // TODO: Support more than one callback here.
  void RegisterAllEvents(NamedEventCallbackFn const& named_callback)
  {
    named_callback_ = named_callback;
  }

  void UnregisterAllEvents()
  {
    named_callback_ = nullptr;
  }

private:
  std::map<std::wstring, EventCallbacks> events_;
  NamedEventCallbackFn named_callback_;
};
}
}
