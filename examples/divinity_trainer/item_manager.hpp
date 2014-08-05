// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "item.hpp"
#include "game_object_manager.hpp"

struct ItemManager : GameObjectManager<Item>
{
};

std::vector<Item*> DumpItemArray(hadesmem::Process const& process,
                                 Item** ptr_array,
                                 std::uint16_t* type_id_array,
                                 std::uint32_t array_len);

void DumpItemManager(hadesmem::Process const& process, std::uint8_t* base);
