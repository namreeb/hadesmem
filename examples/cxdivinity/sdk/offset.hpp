// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

namespace divinity
{

struct DataOffsets
{
  enum : std::uint32_t
  {
    g_character_manager = 0x00E8DE94 - 0x00400000,
    g_item_manager = 0x00E8DF7C - 0x00400000,
    g_inventory_manager = 0x00E8DF90 - 0x00400000,
    g_translated_string_repository = 0x00E2DE38 - 0x00400000,
  };
};

struct FunctionOffsets
{
  enum : std::uint32_t
  {
    g_tri_string_pair_poly_constructor = 0x0042DCD0 - 0x00400000,
    g_tri_string_pair_poly_destructor = 0x00474400 - 0x00400000,
  };
};
}
