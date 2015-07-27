// Copyright (C) 2010-2015 Joshua Boyce
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
  explicit HookDisabler(bool* flag) noexcept : flag_{flag}
  {
    *flag_ = true;
  }

  HookDisabler(HookDisabler const&) = delete;

  HookDisabler& operator=(HookDisabler const&) = delete;

  ~HookDisabler()
  {
    *flag_ = false;
  }

  bool GetFlag() const noexcept
  {
    return *flag_;
  }

private:
  bool* flag_;
};
}
}
