// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "character.hpp"
#include "static_assert.hpp"

namespace divinity
{

struct CharacterPartyManagerCharacterVector
{
  Character** characters_;
  int capacity_;
  int size_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(
  sizeof(CharacterPartyManagerCharacterVector) == 0xC);

struct CharacterPartyManager
{
  int field_0;
  int field_4;
  int field_8;
  CharacterPartyManagerCharacterVector unknown_characters_;
  int field_18;
  CharacterPartyManagerCharacterVector party_members_;
  int field_28[14];
  int party_xp_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterPartyManager) == 0x64);

struct CharacterManager
{
  void* vtable_;
  int field_4;
  int field_8;
  Character** character_ptr_array_;
  int field_10;
  int character_ptr_array_len_;
  int field_18;
  int field_1C;
  int field_20;
  int field_24;
  std::uint16_t* character_type_id_array_;
  int field_2C[28];
  CharacterPartyManager party_manager_;
  int field_100;
  int field_104;
  int field_108;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterManager) == 0x10C);
}

divinity::CharacterManager* GetCharacterManager();

void DumpCharacterManager();

void DumpCharacterManagerPartyManager();

std::vector<divinity::Character*> GetCharactersByName(std::wstring const& name);
