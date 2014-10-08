// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "inventory.hpp"

#include <hadesmem/detail/trace.hpp>

void DumpInventory(divinity::Inventory* inventory, int debug_id)
{
  (void)debug_id;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Got inventory %d: %p.", debug_id, inventory);

  HADESMEM_DETAIL_TRACE_FORMAT_A("Unknown Handle: %08X",
                                 inventory->unknown_handle_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Equipment Slots: %02X",
                                 inventory->equipment_slots_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Parent Handle: %08X",
                                 inventory->parent_handle_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Gold: %d", inventory->gold_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Items::Capacity: %u",
                                 inventory->items_.capacity_);
  if (inventory->items_.capacity_)
  {
    for (std::uint32_t i = 0; i < inventory->items_.capacity_; ++i)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Item %u: %08X", i, inventory->items_.item_handles_[i]);
    }
  }
}
