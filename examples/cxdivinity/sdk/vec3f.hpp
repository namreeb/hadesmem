// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/detail/static_assert_x86.hpp>

namespace divinity
{

struct Vec3f
{
  float x_;
  float y_;
  float z_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(Vec3f) == 0xC);
}
