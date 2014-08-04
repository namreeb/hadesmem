// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "item.hpp"

struct ItemManager
{
  int field_0;
  int field_4;
  int field_8;
  Item **item_ptr_array_;
  int field_10;
  int item_ptr_array_len_;
  int field_18[4];
  WORD *item_type_id_array_;
  int field_2C[200];
};
