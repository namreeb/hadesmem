// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "dump.hpp"

#include <cstdint>

#include <hadesmem/detail/self_path.hpp>

#include "sdk/character_manager.hpp"
#include "sdk/offset.hpp"

void DumpFullInfoCallback()
{
  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const character_manager =
    *reinterpret_cast<divinity::CharacterManager**>(
      base + divinity::DataOffsets::g_character_manager);
  DumpCharacterManager(character_manager);
}

void DumpPartyInfoCallback()
{
  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const character_manager =
    *reinterpret_cast<divinity::CharacterManager**>(
    base + divinity::DataOffsets::g_character_manager);
  DumpCharacterManagerPartyManager(character_manager);
}
