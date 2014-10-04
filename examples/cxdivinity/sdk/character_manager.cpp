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
