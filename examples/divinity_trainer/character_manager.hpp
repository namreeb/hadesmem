// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <vector>

#include <hadesmem/process.hpp>

#include "character.hpp"
#include "game_object_manager.hpp"

struct CharacterPartyManager
{
  int field_0[7];
  Character** party_member_ptr_array_;
  int field_20;
  int party_member_ptr_array_len_;
  int field_28[14];
  int party_xp_;
};

struct CharacterManager : GameObjectManager<Character>
{
  CharacterPartyManager party_manager_;
  int field_100[100];
};

std::vector<Character*> DumpCharacterArray(hadesmem::Process const& process,
                                           Character** ptr_array,
                                           std::uint16_t* type_id_array,
                                           std::uint32_t array_len);

void DumpCharacterManager(hadesmem::Process const& process, std::uint8_t* base);
