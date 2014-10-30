// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace detail
{
class RecursionProtector
{
public:
  explicit RecursionProtector(bool* in_hook) HADESMEM_DETAIL_NOEXCEPT
    : in_hook_{in_hook}
  {
  }

  ~RecursionProtector()
  {
    Revert();
  }

  void Set() HADESMEM_DETAIL_NOEXCEPT
  {
    *in_hook_ = true;
  }

  void Revert() HADESMEM_DETAIL_NOEXCEPT
  {
    *in_hook_ = false;
  }

private:
  bool* in_hook_;
};
}
}
