// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "inventory.hpp"
#include "game_object_manager.hpp"

struct InventoryManager : GameObjectManager<Inventory>
{
};

std::vector<Inventory*> DumpInventoryArray(hadesmem::Process const& process,
                                           Inventory** ptr_array,
                                           std::uint16_t* type_id_array,
                                           std::uint32_t array_len);

void DumpInventoryManager(hadesmem::Process const& process, std::uint8_t* base);
