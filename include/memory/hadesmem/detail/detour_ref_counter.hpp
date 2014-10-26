// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/static_assert.hpp>

namespace hadesmem
{

namespace detail
{

template <typename T> class DetourRefCounter
{
public:
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value);

  DetourRefCounter(std::atomic<T>& ref_count) HADESMEM_DETAIL_NOEXCEPT
    : ref_count_{&ref_count}
  {
    ++(*ref_count_);
  }

  DetourRefCounter(DetourRefCounter const&) = delete;

  DetourRefCounter& operator=(DetourRefCounter const&) = delete;

  DetourRefCounter(DetourRefCounter&& other) HADESMEM_DETAIL_NOEXCEPT
    : ref_count_(other.ref_count_)
  {
    other.ref_count_ = nullptr;
  }

  DetourRefCounter& operator=(DetourRefCounter&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    ref_count_ = other.ref_count_;
    other.ref_count_ = nullptr;

    return *this;
  }

  ~DetourRefCounter()
  {
    if (ref_count_)
    {
      --(*ref_count_);
    }
  }

private:
  std::atomic<T>* ref_count_;
};

template <typename T>
DetourRefCounter<T> MakeDetourRefCounter(std::atomic<T>& ref_count)
{
  return DetourRefCounter<T>{ref_count};
}
}
}
