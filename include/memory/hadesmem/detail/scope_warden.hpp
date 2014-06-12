// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <exception>
#include <memory>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/trace.hpp>

namespace hadesmem
{

namespace detail
{

template <typename F> class ScopeWarden
{
public:
  explicit ScopeWarden(F& f) HADESMEM_DETAIL_NOEXCEPT : f_(std::addressof(f))
  {
  }

  void Dismiss() HADESMEM_DETAIL_NOEXCEPT
  {
    f_ = nullptr;
  }

  explicit ScopeWarden(F&&) = delete;

  ScopeWarden(const ScopeWarden&) = delete;

  ScopeWarden& operator=(const ScopeWarden&) = delete;

  ~ScopeWarden()
  {
    if (f_)
    {
      try
      {
        (*f_)();
      }
      catch (...)
      {
        HADESMEM_DETAIL_TRACE_A(
          boost::current_exception_diagnostic_information().c_str());
        HADESMEM_DETAIL_ASSERT(false);
        std::terminate();
      }
    }
  }

private:
  F* f_;
};
}
}

#define HADESMEM_DETAIL_SCOPE_WARDEN(NAME, ...)                                \
  auto xx##NAME##xx = [&]()                                                    \
  {                                                                            \
    __VA_ARGS__                                                                \
  };                                                                           \
  ::hadesmem::detail::ScopeWarden<decltype(xx##NAME##xx)> NAME(xx##NAME##xx)
