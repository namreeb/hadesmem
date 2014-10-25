// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include <hadesmem/detail/static_assert_x86.hpp>

namespace divinity
{

struct InventoryItems
{
  int field_0;
  unsigned int* item_handles_;
  int field_8;
  unsigned int capacity_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(InventoryItems) == 0x10);

struct Inventory
{
  virtual ~Inventory() = 0;

  virtual bool Serialize(int a2) = 0;

  int field_4;
  std::int16_t field_8;
  std::int16_t field_A;
  int field_C;
  int field_10;
  int field_14;
  int field_18;
  unsigned int unknown_handle_;
  std::uint8_t equipment_slots_;
  char field_21;
  char field_22;
  char field_23;
  unsigned int parent_handle_;
  bool global_;
  char field_29;
  char field_2A;
  char field_2B;
  int gold_;
  int field_30;
  InventoryItems items_;
  int field_44;
  int field_48;
  int field_4C;
  int field_50;
  int field_54;
  int field_58;
  int field_5C;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(Inventory) == 0x60);
}

void DumpInventory(divinity::Inventory* inventory, int debug_id);
