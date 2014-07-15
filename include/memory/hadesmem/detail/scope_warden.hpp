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

  ScopeWarden(ScopeWarden const&) = delete;

  ScopeWarden& operator=(ScopeWarden const&) = delete;

  ScopeWarden(ScopeWarden&& other) HADESMEM_DETAIL_NOEXCEPT : f_(other.f_)
  {
    other.f_ = nullptr;
  }

  ScopeWarden& operator=(ScopeWarden&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    Cleanup();
    std::swap(f_, other.f_);
  }

  ~ScopeWarden()
  {
    Cleanup();
  }

private:
  void Cleanup() HADESMEM_DETAIL_NOEXCEPT
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

      f_ = nullptr;
    }
  }

  F* f_;
};

template <typename F> inline ScopeWarden<F> MakeScopeWarden(F& f)
{
  return ScopeWarden<F>{f};
}
}
}
