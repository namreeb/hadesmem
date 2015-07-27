// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace detail
{
class RecursionProtector
{
public:
  explicit RecursionProtector(std::int32_t* in_hook) noexcept
    : in_hook_{in_hook}
  {
    Set();
  }

  ~RecursionProtector()
  {
    Revert();
  }

  void Set() noexcept
  {
    ++*in_hook_;
  }

  void Revert() noexcept
  {
    --*in_hook_;
  }

private:
  std::int32_t* in_hook_;
};
}
}
