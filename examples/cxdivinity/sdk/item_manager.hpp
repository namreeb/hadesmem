// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "item.hpp"
#include "static_assert.hpp"

namespace divinity
{

struct ItemManager
{
  void* vtable_;
  int field_4;
  int field_8;
  Item** item_ptr_array_;
  int field_10;
  int item_ptr_array_len_;
  int field_18[4];
  std::uint16_t* item_type_id_array_;
  int field_2C[31];
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(ItemManager) == 0xA8);
}

divinity::ItemManager* GetItemManager();

void DumpItemManager();

std::vector<divinity::Item*> GetItemsByName(std::wstring const& name);
