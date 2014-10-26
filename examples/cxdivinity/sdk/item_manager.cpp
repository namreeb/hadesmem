// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "item_manager.hpp"

#include <windows.h>

#include <hadesmem/detail/trace.hpp>

#include "offset.hpp"

divinity::ItemManager* GetItemManager()
{
  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const item_manager = *reinterpret_cast<divinity::ItemManager**>(
                              base + divinity::DataOffsets::g_item_manager);
  return item_manager;
}

void DumpItemManager()
{
  auto const item_manager = GetItemManager();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Got item manager: %p.", item_manager);
  for (int i = 0; i < item_manager->item_ptr_array_len_; ++i)
  {
    if (auto item = item_manager->item_ptr_array_[i])
    {
      DumpItem(item, i);
    }
  }
}

std::vector<divinity::Item*> GetItemsByName(std::wstring const& name)
{
  auto const item_manager = GetItemManager();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Got item manager: %p.", item_manager);
  std::vector<divinity::Item*> items;
  for (int i = 0; i < item_manager->item_ptr_array_len_; ++i)
  {
    if (auto item = item_manager->item_ptr_array_[i])
    {
      auto const cur_name = GetDisplayName(item);
      if (cur_name == name)
      {
        items.push_back(item);
      }
    }
  }

  return items;
}
