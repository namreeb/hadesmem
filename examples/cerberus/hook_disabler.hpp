// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/trace.hpp>

namespace hadesmem
{
namespace cerberus
{
class HookDisabler
{
public:
  explicit HookDisabler(bool* flag) HADESMEM_DETAIL_NOEXCEPT : flag_{flag}
  {
    *flag_ = true;
  }

  HookDisabler(HookDisabler const&) = delete;

  HookDisabler& operator=(HookDisabler const&) = delete;

  ~HookDisabler()
  {
    *flag_ = false;
  }

  bool GetFlag() const HADESMEM_DETAIL_NOEXCEPT
  {
    return *flag_;
  }

private:
  bool* flag_;
};
}
}
