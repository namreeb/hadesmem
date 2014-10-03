// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include "inventory.hpp"
#include "static_assert.hpp"

namespace divinity
{

struct InventoryManager
{
  void* vtable_;
  int field_4;
  int field_8;
  Inventory** inventory_ptr_array_;
  int field_10;
  int inventory_ptr_array_len_;
  int field_18;
  int field_1C;
  int field_20;
  int field_24;
  std::uint16_t* inventory_type_id_array_;
  int field_2C[27];
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(InventoryManager) == 0x98);
}
