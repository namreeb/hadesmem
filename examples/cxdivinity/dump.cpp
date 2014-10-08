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
#include "sdk/translated_string_repository.hpp"

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

  DumpCharacterManager();
}

void DumpPartyInfo()
{
  HADESMEM_DETAIL_TRACE_A("Dumping party info.");

  DumpCharacterManagerPartyManager();
}

void DumpItemInfo()
{
  HADESMEM_DETAIL_TRACE_A("Dumping item info.");

  DumpItemManager();
}

void DumpInventoryInfo()
{
  HADESMEM_DETAIL_TRACE_A("Dumping inventory info.");

  DumpInventoryManager();
}

void DumpStringInfo()
{
  HADESMEM_DETAIL_TRACE_A("Dumping string info.");

  DumpTranslatedStringRepository();
}

void DumpNamedGameObjectInfo(std::wstring const& name)
{
  HADESMEM_DETAIL_TRACE_A("Dumping characters by name.");

  auto const characters = GetCharactersByName(name);
  for (std::size_t i = 0; i < characters.size(); ++i)
  {
    DumpCharacter(characters[i], static_cast<int>(i));
  }

  HADESMEM_DETAIL_TRACE_A("Dumping items by name.");

  auto const items = GetItemsByName(name);
  for (std::size_t i = 0; i < items.size(); ++i)
  {
    DumpItem(items[i], static_cast<int>(i));
  }
}
