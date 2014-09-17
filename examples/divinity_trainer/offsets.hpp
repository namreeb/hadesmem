// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

struct Offsets
{
  enum : std::uint32_t
  {
    g_character_manager = 0x00E8DE94 - 0x00400000,
    g_item_manager = 0x00E8DF7C - 0x00400000,
    g_inventory_manager = 0x00E8DF90 - 0x00400000,
    g_translated_string_repository = 0x00E2DE38 - 0x00400000,
  };
};
