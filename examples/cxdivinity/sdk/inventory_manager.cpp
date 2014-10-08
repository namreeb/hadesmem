// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "inventory_manager.hpp"

#include <windows.h>

#include <hadesmem/detail/trace.hpp>

#include "offset.hpp"

divinity::InventoryManager* GetInventoryManager()
{
  auto const base =
    reinterpret_cast<std::uint8_t*>(::GetModuleHandleW(nullptr));
  auto const inventory_manager =
    *reinterpret_cast<divinity::InventoryManager**>(
      base + divinity::DataOffsets::g_inventory_manager);
  return inventory_manager;
}

void DumpInventoryManager()
{
  auto const inventory_manager = GetInventoryManager();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Got inventory manager: %p.",
                                 inventory_manager);
  for (int i = 0; i < inventory_manager->inventory_ptr_array_len_; ++i)
  {
    if (auto inventory = inventory_manager->inventory_ptr_array_[i])
    {
      DumpInventory(inventory, i);
    }
  }
}
