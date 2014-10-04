// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "character_manager.hpp"

#include <hadesmem/detail/trace.hpp>

void DumpCharacterManager(divinity::CharacterManager* character_manager)
{
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

void DumpCharacterManagerPartyManager(
  divinity::CharacterManager* character_manager)
{
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
