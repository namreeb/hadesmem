// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "inventory.hpp"

#include <cstdio>

#include <hadesmem/read.hpp>

void DumpInventory(hadesmem::Process const& process, Inventory* inventory_ptr)
{
  auto const inventory =
    hadesmem::ReadUnsafe<Inventory>(process, inventory_ptr);
  printf("Parent: %08X\n", inventory.parent_);
  printf("Gold: %d\n", inventory.gold_);
  printf("NumItems: %d\n", inventory.capacity_);
  if (inventory.capacity_)
  {
    auto const item_handles = hadesmem::ReadVector<std::uint32_t>(process, inventory.item_handles_, inventory.capacity_);
    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(item_handles.size()); ++i)
    {
      printf("Item %u: %08X\n", i, item_handles[i]);
    }
  }
}
