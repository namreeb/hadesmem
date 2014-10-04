// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "dump.hpp"

#include <cstdint>

#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/trace.hpp>

#include "sdk/character_manager.hpp"
#include "sdk/inventory_manager.hpp"
#include "sdk/item_manager.hpp"
#include "sdk/offset.hpp"

void DumpFullInfo()
{
  DumpCharacterInfo();

  DumpPartyInfo();

  DumpItemInfo();

  DumpInventoryInfo();
}

void DumpCharacterInfo()
{
  HADESMEM_DETAIL_TRACE_A("Dumping character info.");

  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const character_manager =
    *reinterpret_cast<divinity::CharacterManager**>(
    base + divinity::DataOffsets::g_character_manager);
  DumpCharacterManager(character_manager);
}

void DumpPartyInfo()
{
  HADESMEM_DETAIL_TRACE_A("Dumping party info.");

  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const character_manager =
    *reinterpret_cast<divinity::CharacterManager**>(
    base + divinity::DataOffsets::g_character_manager);
  DumpCharacterManagerPartyManager(character_manager);
}

void DumpItemInfo()
{
  HADESMEM_DETAIL_TRACE_A("Dumping item info.");

  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const item_manager =
    *reinterpret_cast<divinity::ItemManager**>(
    base + divinity::DataOffsets::g_item_manager);
  DumpItemManager(item_manager);
}

void DumpInventoryInfo()
{
  HADESMEM_DETAIL_TRACE_A("Dumping inventory info.");

  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const inventory_manager =
    *reinterpret_cast<divinity::InventoryManager**>(
    base + divinity::DataOffsets::g_inventory_manager);
  DumpInventoryManager(inventory_manager);
}
