// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "character_manager.hpp"

#include <cstdio>

#include <hadesmem/read.hpp>

#include "offsets.hpp"

std::vector<Character*> DumpCharacterArray(hadesmem::Process const& process,
                                           Character** ptr_array,
                                           std::uint16_t* type_id_array,
                                           std::uint32_t array_len)
{
  std::vector<Character*> characters;
  characters.reserve(array_len);

  auto character_ptr_array =
    hadesmem::ReadVector<Character*>(process, ptr_array, array_len);
  auto const character_type_id_array =
    type_id_array
      ? hadesmem::ReadVector<std::uint16_t>(process, type_id_array, array_len)
      : std::vector<std::uint16_t>();
  for (std::uint32_t i = 0; i < character_ptr_array.size(); ++i)
  {
    auto const character_ptr = character_ptr_array[i];
    if (type_id_array)
    {
      printf("Character %X: %p (Type: %04X)\n",
             i,
             static_cast<void*>(character_ptr),
             character_type_id_array[i]);
    }
    else
    {
      printf("Character %X: %p\n", i, static_cast<void*>(character_ptr));
    }

    if (character_ptr != 0)
    {
      DumpCharacter(process, character_ptr);
      characters.push_back(character_ptr);
    }
    else
    {
      printf("  Empty character slot.\n");
    }

    printf("\n");
  }

  return characters;
}

void DumpCharacterManager(hadesmem::Process const& process, std::uint8_t* base)
{
  auto const character_manager_ptr = hadesmem::Read<CharacterManager*>(
    process, base + Offsets::g_character_manager);
  printf("CharacterManager: %p\n", static_cast<void*>(character_manager_ptr));
  auto const character_manager =
    hadesmem::Read<CharacterManager>(process, character_manager_ptr);
  printf("CharacterManager::CharacterPtrArray: %p\n",
         static_cast<void*>(character_manager.object_ptr_array_));
  printf("CharacterManager::CharacterPtrArrayLen: %X\n",
         character_manager.object_ptr_array_len_);
  printf("CharacterManager::CharacterTypeIdArray: %p\n",
         static_cast<void*>(character_manager.object_type_id_array_));
  printf("CharacterManager::PartyMemberPtrArray: %p\n",
         static_cast<void*>(
           character_manager.party_manager_.party_member_ptr_array_));
  printf("CharacterManager::PartyMemberPtrArrayLen: %X\n",
         character_manager.party_manager_.party_member_ptr_array_len_);
  printf("\n");

  printf("Dumping character array.\n\n");
  DumpCharacterArray(process,
                     character_manager.object_ptr_array_,
                     character_manager.object_type_id_array_,
                     character_manager.object_ptr_array_len_);

  printf("Dumping party array.\n\n");
  DumpCharacterArray(
    process,
    character_manager.party_manager_.party_member_ptr_array_,
    nullptr,
    character_manager.party_manager_.party_member_ptr_array_len_);
}
