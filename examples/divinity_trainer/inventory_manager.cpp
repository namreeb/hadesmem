// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "inventory_manager.hpp"

#include <cstdio>

#include <hadesmem/read.hpp>

#include "offsets.hpp"

std::vector<Inventory*> DumpItemArray(hadesmem::Process const& process,
                                      Inventory** ptr_array,
                                      std::uint16_t* type_id_array,
                                      std::uint32_t array_len)
{
  std::vector<Inventory*> inventories;
  inventories.reserve(array_len);

  auto inventory_ptr_array =
    hadesmem::ReadVector<Inventory*>(process, ptr_array, array_len);
  auto const item_type_id_array =
    type_id_array
      ? hadesmem::ReadVector<std::uint16_t>(process, type_id_array, array_len)
      : std::vector<std::uint16_t>();
  for (std::uint32_t i = 0; i < inventory_ptr_array.size(); ++i)
  {
    auto const inventory_ptr = inventory_ptr_array[i];
    if (type_id_array)
    {
      printf("Inventory %X: %p (Type: %04X)\n",
             i,
             static_cast<void*>(inventory_ptr),
             item_type_id_array[i]);
    }
    else
    {
      printf("Inventory %X: %p\n", i, static_cast<void*>(inventory_ptr));
    }

    if (inventory_ptr != 0)
    {
      DumpInventory(process, inventory_ptr);
      inventories.push_back(inventory_ptr);
    }
    else
    {
      printf("  Empty item slot.\n");
    }

    printf("\n");
  }

  return inventories;
}

void DumpInventoryManager(hadesmem::Process const& process, std::uint8_t* base)
{
  auto const inventory_manager_ptr =
    hadesmem::Read<InventoryManager*>(process, base + Offsets::g_inventory_manager);
  auto const inventory_manager =
    hadesmem::Read<InventoryManager>(process, inventory_manager_ptr);
  printf("InventoryManager::InventoryPtrArray: %p\n",
         static_cast<void*>(inventory_manager.object_ptr_array_));
  printf("InventoryManager::InventoryPtrArrayLen: %X\n",
         inventory_manager.object_ptr_array_len_);
  printf("InventoryManager::InventoryTypeIdArray: %p\n",
         static_cast<void*>(inventory_manager.object_type_id_array_));
  printf("\n");

  printf("Dumping inventory array.\n\n");
  DumpItemArray(process,
                inventory_manager.object_ptr_array_,
                inventory_manager.object_type_id_array_,
                inventory_manager.object_ptr_array_len_);
}
