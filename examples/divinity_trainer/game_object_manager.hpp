// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

template <typename T>
struct GameObjectManager
{
  int field_0;
  int field_4;
  int field_8;
  T** object_ptr_array_;
  int field_10;
  int object_ptr_array_len_;
  int field_18;
  int field_1C;
  int field_20;
  int field_24;
  std::uint16_t* object_type_id_array_;
  int field_2C[14];
  int field_64;
  int field_68;
  int field_6C[12];
};
