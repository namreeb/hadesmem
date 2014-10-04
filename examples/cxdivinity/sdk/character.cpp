// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "character.hpp"

#include <hadesmem/detail/trace.hpp>

void DumpCharacter(divinity::Character* character, int debug_id)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Got character %d: %p.", debug_id, character);

  HADESMEM_DETAIL_TRACE_FORMAT_A("Position: %f %f %f",
                                 character->position_.x_,
                                 character->position_.y_,
                                 character->position_.z_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Handle: %08X", character->handle_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Flags: %08X", character->flags_);
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsPlayer: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsPlayer));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsMultiPlayer: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsMultiPlayer));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsPartyMember: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsPartyMember));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsDead: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsDead));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsSummon: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsSummon));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsInvulnerable: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsInvulnerable));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsImmortal: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsImmortal));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsStoryNpc: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsStoryNpc));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::CanFight: %d",
    !!(character->flags_ & divinity::CharacterFlags_CanFight));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsInvisible: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsVisible));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsFloating: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsFloating));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsInFightMode: %d",
    !!(character->flags_ & divinity::CharacterFlags_IsInFightMode));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::CanSpotSneakers: %d",
    !!(character->flags_ & divinity::CharacterFlags_CanSpotSneakers));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::HasReputationEffects: %d",
    !!(character->flags_ & divinity::CharacterFlags_HasReputationEffects));
  HADESMEM_DETAIL_TRACE_FORMAT_A("Level (Region): %s", character->level_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Rotate: {%f %f %f}, {%f %f %f}, {%f %f %f}",
                                 character->character_data_.rotate_[0].x_,
                                 character->character_data_.rotate_[0].y_,
                                 character->character_data_.rotate_[0].z_,
                                 character->character_data_.rotate_[1].x_,
                                 character->character_data_.rotate_[1].y_,
                                 character->character_data_.rotate_[1].z_,
                                 character->character_data_.rotate_[2].x_,
                                 character->character_data_.rotate_[2].y_,
                                 character->character_data_.rotate_[2].z_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Scale: %f",
                                 character->character_data_.scale_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("UUID: %s", character->character_data_.uuid_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("CurrentTemplate: %p",
                                 character->character_data_.current_template_);
  auto const current_character_template =
    character->character_data_.current_template_;
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "CurrentTemplate::Name: %s",
    GetStdStringRaw(&current_character_template->name_));
  DumpTriStringPairPoly("CurrentTemplate::NameData",
                        &current_character_template->name_data_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("CurrentTemplate::Alignment: %s",
                                 current_character_template->alignment_.value_);
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "OriginalTemplate: %p",
    static_cast<void*>(character->character_data_.original_template_));
  auto const original_character_template =
    character->character_data_.original_template_;
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "OriginalTemplate::Name: %s",
    GetStdStringRaw(&original_character_template->name_));
  DumpTriStringPairPoly("OriginalTemplate::NameData",
                        &original_character_template->name_data_);
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "OriginalTemplate::Alignment: %s",
    original_character_template->alignment_.value_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Flags2: %08X",
                                 character->character_data_.flags_2_);
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::Global: %d",
    !!(character->character_data_.flags_2_ & divinity::CharacterFlags2_Global));
  HADESMEM_DETAIL_TRACE_FORMAT_A("Flags::HasOsirisDialog: %d",
                                 !!(character->character_data_.flags_2_ &
                                    divinity::CharacterFlags2_HasOsirisDialog));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::HasDefaultDialog: %d",
    !!(character->character_data_.flags_2_ &
       divinity::CharacterFlags2_HasDefaultDialog));
  HADESMEM_DETAIL_TRACE_FORMAT_A("Flags::TurnEnded: %d",
                                 !!(character->character_data_.flags_2_ &
                                    divinity::CharacterFlags2_TurnEnded));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::TreasureGeneratedForTrader: %d",
    !!(character->character_data_.flags_2_ &
       divinity::CharacterFlags2_TreasureGeneratedForTrader));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::Trader: %d",
    !!(character->character_data_.flags_2_ & divinity::CharacterFlags2_Trader));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsBoss: %d",
    !!(character->character_data_.flags_2_ & divinity::CharacterFlags2_IsBoss));
  HADESMEM_DETAIL_TRACE_FORMAT_A("Flags::ScriptAllowsAoO: %d",
                                 !!(character->character_data_.flags_2_ &
                                    divinity::CharacterFlags2_ScriptAllowsAoO));
  HADESMEM_DETAIL_TRACE_FORMAT_A("Flags::IsResurrected: %d",
                                 !!(character->character_data_.flags_2_ &
                                    divinity::CharacterFlags2_IsResurrected));
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Flags::IsPet: %d",
    !!(character->character_data_.flags_2_ & divinity::CharacterFlags2_IsPet));
  HADESMEM_DETAIL_TRACE_FORMAT_A("Stats: %p",
                                 character->character_data_.stats_);
  auto const character_stats = character->character_data_.stats_;
  DumpTriStringPairPoly("Stats::Field28", &character_stats->field_28);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::Level: %d", character_stats->level_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::HP: %d", character_stats->vitality_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::MP: %d",
                                 character_stats->magic_points_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::AP: %d",
                                 character_stats->action_points_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::BonusAP: %d",
                                 character_stats->bonus_action_points_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::XP: %d", character_stats->experience_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::Reputation: %d",
                                 character_stats->reputation_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Stats::TraitOrder: %08X",
                                 character_stats->trait_order_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("InventoryHandle: %08X",
                                 character->character_data_.inventory_handle_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("MovementMachine: %p",
                                 character->character_data_.movement_machine_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("SteeringMachine: %p",
                                 character->character_data_.steering_machine_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("OsirisController: %p",
                                 character->character_data_.osiris_controller_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("DialogController: %p",
                                 character->character_data_.dialog_controller_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("StatusManager: %p",
                                 character->character_data_.status_manager_);
  auto const character_status_manager =
    character->character_data_.status_manager_;
  HADESMEM_DETAIL_TRACE_FORMAT_A("StatusManager::Statuses::Size: %u",
                                 character_status_manager->statuses_.size_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("StatusManager::Statuses::Capacity: %u",
                                 character_status_manager->statuses_.capacity_);
  for (unsigned int i = 0; i < character_status_manager->statuses_.size_; ++i)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "StatusManager::Statuses::Status %u: 0x%p",
      i,
      character_status_manager->statuses_.statuses_[i]);
  }
  HADESMEM_DETAIL_TRACE_FORMAT_A("SkillManager: %p",
                                 character->character_data_.skill_manager_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("VariableManager: %p",
                                 character->character_data_.variable_manager_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Owner: %08X",
                                 character->character_data_.owner_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("TeamID: %d",
                                 character->character_data_.team_id_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("LifeTime: %f",
                                 character->character_data_.life_time_);
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "TriggerTrapsTimer: %f", character->character_data_.trigger_traps_timer_);
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "SurfaceDistanceCheck: %f",
    character->character_data_.surface_distance_check_);
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "SurfaceTimerCheck: %f", character->character_data_.surface_timer_check_);
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "PreviousSurfaceTileState: %08X",
    character->character_data_.previous_surface_tile_state_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("ReservedPeerId: %d",
                                 character->character_data_.reserved_peer_id_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("OwnerHandle: %08X",
                                 character->character_data_.owner_handle_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("FollowHandle: %08X",
                                 character->character_data_.follow_handle_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("EnemyHandle: %08X",
                                 character->character_data_.enemy_handle_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("Enemies: %08X",
                                 character->character_data_.enemies_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("HostileCount: %d",
                                 character->character_data_.hostile_count_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("PlanManager: %p",
                                 character->character_data_.plan_manager_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("PartialAp: %d",
                                 character->character_data_.partial_ap_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("CurrentAoO: %d",
                                 character->character_data_.current_aoo_);
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "RegisteredTrigger: %d", character->character_data_.registered_trigger_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("PlayerData: %p",
                                 character->character_data_.player_data_);
  if (character->character_data_.player_data_)
  {
    auto const character_player_data = character->character_data_.player_data_;
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "PlayerData::Attribute: %d",
      character_player_data->upgrade_data_.attribute_points_);
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "PlayerData::Ability: %d",
      character_player_data->upgrade_data_.ability_points_);
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "PlayerData::Talent: %d",
      character_player_data->upgrade_data_.talent_points_);

    if (character_player_data->player_custom_data_.initialized_)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(
        L"PlayerData::PlayerCustomData::Name: %s",
        GetStdStringRaw(&character_player_data->player_custom_data_.name_));
      DumpTriStringPairPoly(
        "PlayerData::PlayerCustomData::NameData",
        &character_player_data->player_custom_data_.name_data_);
    }
  }
  HADESMEM_DETAIL_TRACE_FORMAT_A("Gender: %d",
                                 character->character_data_.gender_);
  HADESMEM_DETAIL_TRACE_FORMAT_A("CombatData: %p",
                                 character->character_data_.combat_data_);
  if (character->character_data_.custom_kickstarter_name_)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(
      L"CustomKickStarterName: %s",
      GetStdStringRaw(character->character_data_.custom_kickstarter_name_));
  }
}
