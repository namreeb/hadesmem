// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "character_manager.hpp"

#include <windows.h>

#include <hadesmem/detail/trace.hpp>

#include "offset.hpp"

divinity::CharacterManager* GetCharacterManager()
{
  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const character_manager =
    *reinterpret_cast<divinity::CharacterManager**>(
      base + divinity::DataOffsets::g_character_manager);
  return character_manager;
}

void DumpCharacterManager()
{
  auto const character_manager = GetCharacterManager();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Got character manager: %p.",
                                 character_manager);
  for (int i = 0; i < character_manager->character_ptr_array_len_; ++i)
  {
    if (auto character = character_manager->character_ptr_array_[i])
    {
      DumpCharacter(character, i);
    }
  }
}

void DumpCharacterManagerPartyManager()
{
  auto const character_manager = GetCharacterManager();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Got character manager: %p.",
                                 character_manager);

  HADESMEM_DETAIL_TRACE_A("Dumping party member vector.");
  auto const party_manager_vec =
    &character_manager->party_manager_.party_members_;
  for (int i = 0; i < party_manager_vec->size_; ++i)
  {
    if (auto character = party_manager_vec->characters_[i])
    {
      DumpCharacter(character, i);
    }
  }

  HADESMEM_DETAIL_TRACE_A("Dumping unknown party member vector.");
  auto const unknown_party_manager_vec =
    &character_manager->party_manager_.party_members_;
  for (int i = 0; i < unknown_party_manager_vec->size_; ++i)
  {
    if (auto character = unknown_party_manager_vec->characters_[i])
    {
      DumpCharacter(character, i);
    }
  }
}

std::vector<divinity::Character*> GetCharactersByName(std::wstring const& name)
{
  auto const character_manager = GetCharacterManager();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Got character manager: %p.",
                                 character_manager);
  std::vector<divinity::Character*> characters;
  for (int i = 0; i < character_manager->character_ptr_array_len_; ++i)
  {
    if (auto character = character_manager->character_ptr_array_[i])
    {
      auto const cur_name = GetDisplayName(character);
      if (cur_name == name)
      {
        characters.push_back(character);
      }
    }
  }

  return characters;
}
