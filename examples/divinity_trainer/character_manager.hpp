// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "character.hpp"

struct CharacterPartyManager
{
  int field_0[7];
  Character** party_member_ptr_array_;
  int field_20;
  int party_member_ptr_array_len_;
  int field_28[14];
  int party_xp_;
};

struct CharacterManager
{
  int field_0;
  int field_4;
  int field_8;
  Character** character_ptr_array_;
  int field_10;
  int character_ptr_array_len_;
  int field_18;
  int field_1C;
  int field_20;
  int field_24;
  WORD* character_type_id_array_;
  int field_2C[14];
  int field_64;
  int field_68;
  int field_6C[12];
  CharacterPartyManager party_manager_;
  int field_100[50];
};
