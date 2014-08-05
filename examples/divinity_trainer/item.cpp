// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "item.hpp"

#include <cstdio>

void DumpItem(hadesmem::Process const& process, Item* item_ptr)
{
  auto const item = hadesmem::ReadUnsafe<Item>(process, item_ptr);
  printf("Position: %f %f %f\n",
         item.position_.x_,
         item.position_.y_,
         item.position_.z_);
  printf("Handle: %08X\n", item.handle_);
  printf("Flags: %08X\n", item.flags_);
  printf("Flags::ItemFlags_IsVisible: %d\n",
         !!(item.flags_ & ItemFlags::ItemFlags_IsVisible));
  printf("Scale: %f\n", item.scale_);
  printf("UUID: %s\n", hadesmem::ReadString<char>(process, item.uuid_).c_str());
  printf("Flags2: %08X\n", item.flags_2_);
  printf("Velocity: %f %f %f\n",
         item.velocity_.x_,
         item.velocity_.y_,
         item.velocity_.z_);
  printf("CurrentTemplate: %p\n", static_cast<void*>(item.current_template_));
  auto const current_item_template =
    hadesmem::Read<ItemTemplate>(process, item.current_template_);
  printf(
    "CurrentTemplate::UUID: %s\n",
    hadesmem::ReadString<char>(process, current_item_template.uuid_).c_str());
  printf("CurrentTemplate::Name: %s\n",
         GetString(process, current_item_template.name_).c_str());
  printf("OriginalTemplate: %p\n", static_cast<void*>(item.original_template_));
  auto const original_item_template =
    hadesmem::Read<ItemTemplate>(process, item.original_template_);
  printf(
    "OriginalTemplate::UUID: %s\n",
    hadesmem::ReadString<char>(process, original_item_template.uuid_).c_str());
  printf("OriginalTemplate::Name: %s\n",
         GetString(process, original_item_template.name_).c_str());
  printf("Stats: %p\n", static_cast<void*>(item.stats_));
  if (item.stats_)
  {
    auto const item_stats = hadesmem::Read<ItemStats>(process, item.stats_);
    printf("Stats::Level: %d\n", item_stats.level_);
    printf("Stats::IsIdentified: %d\n", item_stats.is_identified_);
    printf("Stats::Durability: %d\n", item_stats.durability_);
    printf("Stats::DurabilityCounter: %d\n", item_stats.durability_counter_);
    printf("Stats::RepairDurabilityPenalty: %d\n",
           item_stats.repair_durability_penalty_);
    printf("Stats::ItemType: %s\n", hadesmem::ReadString<char>(process, item_stats.item_type_).c_str());
  }
  printf("Inventory: %08X\n", item.inventory_);
  printf("Parent: %08X\n", item.parent_);
  printf("Slot: %d\n", item.slot_);
  printf("Amount: %d\n", item.amount_);
  printf("Vitality: %d\n", item.vitality_);
  printf("Key: %08X\n", item.key_);
  printf("LockLevel: %d\n", item.lock_level_);
  printf("SurfaceCheckTimer: %f\n", item.surface_check_timer_);
  printf("LifeTime: %f\n", item.life_time_);
  printf("ItemMachine: %p\n", item.item_machine_);
  printf("PlanManager: %p\n", item.plan_manager_);
  printf("VariableManager: %p\n", item.variable_manager_);
  printf("StatusManager: %p\n", item.status_manager_);
  printf("Owner: %08X\n", item.owner_);
}
