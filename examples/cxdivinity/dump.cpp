// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "dump.hpp"

#include <cstdint>

#include <hadesmem/detail/self_path.hpp>

#include "sdk/character_manager.hpp"
#include "sdk/offset.hpp"

namespace
{

void DumpCharacterManager()
{
  auto base = reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  divinity::CharacterManager* character_manager =
    *reinterpret_cast<divinity::CharacterManager**>(
      base + divinity::DataOffsets::g_character_manager);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Got character manager: %p.",
                                 character_manager);
  for (int i = 0; i < character_manager->character_ptr_array_len_; ++i)
  {
    auto character = character_manager->character_ptr_array_[i];
    HADESMEM_DETAIL_TRACE_FORMAT_A("Got character: %p.", character);
    if (character)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A("  UUID: %s.",
                                     character->character_data_.uuid_);
    }
  }
}
}

void DumpInfoCallback()
{
  DumpCharacterManager();
}
