// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/trace.hpp>

namespace hadesmem
{
namespace cerberus
{
class HookCounter
{
public:
  explicit HookCounter(std::uint32_t* counter) HADESMEM_DETAIL_NOEXCEPT
    : counter_{counter}
  {
    ++*counter_;
  }

  HookCounter(HookCounter const&) = delete;

  HookCounter& operator=(HookCounter const&) = delete;

  ~HookCounter()
  {
    --*counter_;
  }

  std::uint32_t GetCount() const HADESMEM_DETAIL_NOEXCEPT
  {
    return *counter_;
  }

private:
  std::uint32_t* counter_;
};
}
}
