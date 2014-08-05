// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "item_manager.hpp"

#include <cstdio>

#include <hadesmem/read.hpp>

#include "offsets.hpp"

std::vector<Item*> DumpItemArray(hadesmem::Process const& process,
                                 Item** ptr_array,
                                 std::uint16_t* type_id_array,
                                 std::uint32_t array_len)
{
  std::vector<Item*> items;
  items.reserve(array_len);

  auto item_ptr_array =
    hadesmem::ReadVector<Item*>(process, ptr_array, array_len);
  auto const item_type_id_array =
    type_id_array
      ? hadesmem::ReadVector<std::uint16_t>(process, type_id_array, array_len)
      : std::vector<std::uint16_t>();
  for (std::uint32_t i = 0; i < item_ptr_array.size(); ++i)
  {
    auto const item_ptr = item_ptr_array[i];
    if (type_id_array)
    {
      printf("Item %X: %p (Type: %04X)\n",
             i,
             static_cast<void*>(item_ptr),
             item_type_id_array[i]);
    }
    else
    {
      printf("Item %X: %p\n", i, static_cast<void*>(item_ptr));
    }

    if (item_ptr != 0)
    {
      DumpItem(process, item_ptr);
      items.push_back(item_ptr);
    }
    else
    {
      printf("  Empty item slot.\n");
    }

    printf("\n");
  }

  return items;
}

void DumpItemManager(hadesmem::Process const& process, std::uint8_t* base)
{
  auto const item_manager_ptr =
    hadesmem::Read<ItemManager*>(process, base + Offsets::g_item_manager);
  auto const item_manager =
    hadesmem::Read<ItemManager>(process, item_manager_ptr);
  printf("ItemManager::ItemPtrArray: %p\n",
         static_cast<void*>(item_manager.object_ptr_array_));
  printf("ItemManager::ItemPtrArrayLen: %X\n",
         item_manager.object_ptr_array_len_);
  printf("ItemManager::ItemTypeIdArray: %p\n",
         static_cast<void*>(item_manager.object_type_id_array_));
  printf("\n");

  printf("Dumping item array.\n\n");
  DumpItemArray(process,
                item_manager.object_ptr_array_,
                item_manager.object_type_id_array_,
                item_manager.object_ptr_array_len_);
}
