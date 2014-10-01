// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "static_assert.hpp"
#include "tri_string.hpp"

namespace divinity
{

template <typename T> struct FlaggedT
{
  T value_;
  bool initialized_;
  char field_5;
  char field_6;
  char field_7;
};

using FlaggedCString = FlaggedT<char*>;

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(FlaggedCString) == 0x8);

using FlaggedFloat = FlaggedT<float>;

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(FlaggedFloat) == 0x8);

using FlaggedDword = FlaggedT<int>;

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(FlaggedDword) == 0x8);

using FlaggedTriStringPairPoly = FlaggedT<TriStringPairPoly>;

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(FlaggedTriStringPairPoly) == 0x88);

template <> struct FlaggedT<bool>
{
  bool value_;
  bool initialized_;
};

using FlaggedBool = FlaggedT<bool>;

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(FlaggedBool) == 0x2);
}
