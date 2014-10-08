// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "inventory_manager.hpp"

#include <hadesmem/detail/trace.hpp>

void DumpInventoryManager(divinity::InventoryManager* inventory_manager)
{
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
