// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "item.hpp"

#include <hadesmem/detail/trace.hpp>

void DumpItem(divinity::Item* item, int debug_id)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Got item %d: %p.", debug_id, item);

  HADESMEM_DETAIL_TRACE_FORMAT_A("Position: %f %f %f",
                                 item->position_.x_,
                                 item->position_.y_,
                                 item->position_.z_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Handle: %08X", item->handle_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Flags: %08X", item->flags_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Flags::ItemFlags_IsVisible: %d",
    !!(item->flags_ & divinity::ItemFlags::ItemFlags_IsVisible));
  HADESMEM_DETAIL_TRACE_FORMAT_A("Level (Region): %s", item->level_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Scale: %f", item->item_data_.scale_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("UUID: %s", item->item_data_.uuid_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Flags2: %08X", item->item_data_.flags_2_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Velocity: %f %f %f",
    item->item_data_.velocity_.x_,
    item->item_data_.velocity_.y_,
    item->item_data_.velocity_.z_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("CurrentTemplate: %p", item->item_data_.current_template_);
  auto const current_item_template = item->item_data_.current_template_;
  HADESMEM_DETAIL_TRACE_FORMAT_A("CurrentTemplate::UUID: %s", current_item_template->uuid_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("CurrentTemplate::Name: %s", GetStdStringRaw(&current_item_template->name_));
  HADESMEM_DETAIL_TRACE_FORMAT_A("OriginalTemplate: %p", item->item_data_.original_template_);
  auto const original_item_template = item->item_data_.original_template_;
  HADESMEM_DETAIL_TRACE_FORMAT_A("OriginalTemplate::UUID: %s", original_item_template->uuid_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("OriginalTemplate::Name: %s", GetStdStringRaw(&original_item_template->name_));
  HADESMEM_DETAIL_TRACE_FORMAT_A("StatsId: %s", item->item_data_.stats_id_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Stats: %p", item->item_data_.stats_);
  if (item->item_data_.stats_)
  {
    auto const item_stats = item->item_data_.stats_;
    HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::Level: %d", item_stats->level_);
    DumpTriStringPairPoly("Stats::NameData", &item_stats->name_data_);
    HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::IsIdentified: %d", item_stats->is_identified_);
    HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::Durability: %d", item_stats->durability_);
    HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::DurabilityCounter: %d", item_stats->durability_counter_);
    HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::RepairDurabilityPenalty: %d",
      item_stats->repair_durability_penalty_);
    HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::ItemType: %s", item_stats->item_type_);
  }
  HADESMEM_DETAIL_TRACE_FORMAT_A("Inventory: %08X", item->item_data_.inventory_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Parent: %08X", item->item_data_.parent_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Slot: %d", item->item_data_.slot_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Amount: %d", item->item_data_.amount_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Vitality: %d", item->item_data_.vitality_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Key: %s", item->item_data_.key_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("LockLevel: %d", item->item_data_.lock_level_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("SurfaceCheckTimer: %f", item->item_data_.surface_check_timer_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("LifeTime: %f", item->item_data_.life_time_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("ItemMachine: %p", item->item_data_.item_machine_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("PlanManager: %p", item->item_data_.plan_manager_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("VariableManager: %p", item->item_data_.variable_manager_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("StatusManager: %p", item->item_data_.status_manager_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Owner: %08X", item->item_data_.owner_handle_);
}
