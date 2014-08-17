// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "character.hpp"

#include <cstdio>

void DumpGameObjectNameData(hadesmem::Process const& process,
                            GameObjectNameData const& name_data,
                            std::string const& type_name)
{
  printf("%s::Name1::FixedString: %s.\n",
         type_name.c_str(),
         hadesmem::ReadString<char>(process, name_data.name_1_.fixed_narrow_)
           .c_str());
  printf("%s::Name1::StdNarrow: %s.\n",
         type_name.c_str(),
         GetString(process, name_data.name_1_.std_narrow_).c_str());
  printf("%s::Name1::StdWide: %ls.\n",
         type_name.c_str(),
         GetString(process, name_data.name_1_.std_wide_).c_str());

  printf("%s::Name2::FixedString: %s.\n",
         type_name.c_str(),
         hadesmem::ReadString<char>(process, name_data.name_2_.fixed_narrow_)
           .c_str());
  printf("%s::Name2::StdNarrow: %s.\n",
         type_name.c_str(),
         GetString(process, name_data.name_2_.std_narrow_).c_str());
  printf("%s::Name2::StdWide: %ls.\n",
         type_name.c_str(),
         GetString(process, name_data.name_2_.std_wide_).c_str());
}

void DumpCharacter(hadesmem::Process const& process, Character* character_ptr)
{
  auto const character =
    hadesmem::ReadUnsafe<Character>(process, character_ptr);
  printf("Position: %f %f %f\n",
         character.position_.x_,
         character.position_.y_,
         character.position_.z_);
  printf("Handle: %08X\n", character.handle_);
  printf("Flags: %08X\n", character.flags_);
  printf("Flags::IsPlayer: %d\n",
         !!(character.flags_ & CharacterFlags_IsPlayer));
  printf("Flags::IsMultiPlayer: %d\n",
         !!(character.flags_ & CharacterFlags_IsMultiPlayer));
  printf("Flags::IsPartyMember: %d\n",
         !!(character.flags_ & CharacterFlags_IsPartyMember));
  printf("Flags::IsDead: %d\n", !!(character.flags_ & CharacterFlags_IsDead));
  printf("Flags::IsSummon: %d\n",
         !!(character.flags_ & CharacterFlags_IsSummon));
  printf("Flags::IsInvulnerable: %d\n",
         !!(character.flags_ & CharacterFlags_IsInvulnerable));
  printf("Flags::IsImmortal: %d\n",
         !!(character.flags_ & CharacterFlags_IsImmortal));
  printf("Flags::IsStoryNpc: %d\n",
         !!(character.flags_ & CharacterFlags_IsStoryNpc));
  printf("Flags::CanFight: %d\n",
         !!(character.flags_ & CharacterFlags_CanFight));
  printf("Flags::IsInvisible: %d\n",
         !!(character.flags_ & CharacterFlags_IsVisible));
  printf("Flags::IsFloating: %d\n",
         !!(character.flags_ & CharacterFlags_IsFloating));
  printf("Flags::IsInFightMode: %d\n",
         !!(character.flags_ & CharacterFlags_IsInFightMode));
  printf("Flags::CanSpotSneakers: %d\n",
         !!(character.flags_ & CharacterFlags_CanSpotSneakers));
  printf("Flags::HasReputationEffects: %d\n",
         !!(character.flags_ & CharacterFlags_HasReputationEffects));
  printf("Level (Region): %s\n",
         hadesmem::ReadString<char>(process, character.level_).c_str());
  printf("Rotate: {%f %f %f}, {%f %f %f}, {%f %f %f}\n",
         character.rotate_[0].x_,
         character.rotate_[0].y_,
         character.rotate_[0].z_,
         character.rotate_[1].x_,
         character.rotate_[1].y_,
         character.rotate_[1].z_,
         character.rotate_[2].x_,
         character.rotate_[2].y_,
         character.rotate_[2].z_);
  printf("Scale: %f\n", character.scale_);
  printf("UUID: %s\n",
         hadesmem::ReadString<char>(process, character.uuid_).c_str());
  printf("CurrentTemplate: %p\n",
         static_cast<void*>(character.current_template_));
  auto const current_character_template =
    hadesmem::ReadUnsafe<CharacterTemplate>(process,
                                            character.current_template_);
  printf("CurrentTemplate::Name: %s\n",
         GetString(process, current_character_template.name_).c_str());
  DumpGameObjectNameData(process,
                         current_character_template.name_data_,
                         "CurrentTemplate::NameData");
  printf("CurrentTemplate::Alignment: %s\n",
         hadesmem::ReadString<char>(
           process, current_character_template.alignment_).c_str());
  printf("OriginalTemplate: %p\n",
         static_cast<void*>(character.original_template_));
  auto const original_character_template =
    hadesmem::ReadUnsafe<CharacterTemplate>(process,
                                            character.current_template_);
  printf("OriginalTemplate::Name: %s\n",
         GetString(process, original_character_template.name_).c_str());
  DumpGameObjectNameData(process,
                         original_character_template.name_data_,
                         "OriginalTemplate::NameData");
  printf("OriginalTemplate::Alignment: %s\n",
         hadesmem::ReadString<char>(
           process, original_character_template.alignment_).c_str());
  printf("Flags2: %08X\n", character.flags_2_);
  printf("Flags::Global: %d\n",
         !!(character.flags_2_ & CharacterFlags2_Global));
  printf("Flags::HasOsirisDialog: %d\n",
         !!(character.flags_2_ & CharacterFlags2_HasOsirisDialog));
  printf("Flags::HasDefaultDialog: %d\n",
         !!(character.flags_2_ & CharacterFlags2_HasDefaultDialog));
  printf("Flags::TurnEnded: %d\n",
         !!(character.flags_2_ & CharacterFlags2_TurnEnded));
  printf("Flags::TreasureGeneratedForTrader: %d\n",
         !!(character.flags_2_ & CharacterFlags2_TreasureGeneratedForTrader));
  printf("Flags::Trader: %d\n",
         !!(character.flags_2_ & CharacterFlags2_Trader));
  printf("Flags::IsBoss: %d\n",
         !!(character.flags_2_ & CharacterFlags2_IsBoss));
  printf("Flags::ScriptAllowsAoO: %d\n",
         !!(character.flags_2_ & CharacterFlags2_ScriptAllowsAoO));
  printf("Flags::IsResurrected: %d\n",
         !!(character.flags_2_ & CharacterFlags2_IsResurrected));
  printf("Flags::IsPet: %d\n", !!(character.flags_2_ & CharacterFlags2_IsPet));
  printf("Stats: %p\n", static_cast<void*>(character.stats_));
  auto const character_stats =
    hadesmem::Read<CharacterStats>(process, character.stats_);
  printf("Stats::Level: %d\n", character_stats.level_);
  printf("Stats::HP: %d\n", character_stats.vitality_);
  printf("Stats::MP: %d\n", character_stats.magic_points_);
  printf("Stats::AP: %d\n", character_stats.action_points_);
  printf("Stats::BonusAP: %d\n", character_stats.bonus_action_points_);
  printf("Stats::XP: %d\n", character_stats.experience_);
  printf("Stats::Reputation: %d\n", character_stats.reputation_);
  printf("Stats::TraitOrder: %08X\n", character_stats.trait_order_);
  printf("InventoryHandle: %08X\n", character.inventory_handle_);
  printf("MovementMachine: %p\n", character.movement_machine_);
  printf("SteeringMachine: %p\n", character.steering_machine_);
  printf("OsirisController: %p\n", character.osiris_controller_);
  printf("DialogController: %p\n", character.dialog_controller_);
  printf("StatusManager: %p\n", character.status_manager_);
  auto const character_status_manager =
    hadesmem::ReadUnsafe<CharacterStatusManager>(process,
                                                 character.status_manager_);
  printf("StatusManager::NumStatuses: %d\n",
         character_status_manager.num_statuses_);
  printf("SkillManager: %p\n", character.skill_manager_);
  printf("VariableManager: %p\n", character.variable_manager_);
  printf("Owner: %08X\n", character.owner_);
  printf("TeamID: %d\n", character.team_id_);
  printf("LifeTime: %f\n", character.life_time_);
  printf("TriggerTrapsTimer: %f\n", character.trigger_traps_timer_);
  printf("SurfaceDistanceCheck: %f\n", character.surface_distance_check_);
  printf("SurfaceTimerCheck: %f\n", character.surface_timer_check_);
  printf("PreviousSurfaceTileState: %08X\n",
         character.previous_surface_tile_state_);
  printf("ReservedPeerId: %d\n", character.reserved_peer_id_);
  printf("OwnerHandle: %08X\n", character.owner_handle_);
  printf("FollowHandle: %08X\n", character.follow_handle_);
  printf("EnemyHandle: %08X\n", character.enemy_handle_);
  printf("Enemies: %08X\n", character.enemies_);
  printf("HostileCount: %d\n", character.hostile_count_);
  printf("PlanManager: %p\n", character.plan_manager_);
  printf("PartialAp: %d\n", character.partial_ap_);
  printf("CurrentAoO: %d\n", character.current_aoo_);
  printf("RegisteredTrigger: %d\n", character.registered_trigger_);
  printf("PlayerData: %p\n", static_cast<void*>(character.player_data_));
  if (character.player_data_)
  {
    auto const character_player_data =
      hadesmem::ReadUnsafe<CharacterPlayerData>(process,
                                                character.player_data_);
    printf("PlayerData::Attribute: %d\n",
           character_player_data.attribute_points_);
    printf("PlayerData::Ability: %d\n", character_player_data.ability_points_);
    printf("PlayerData::Talent: %d\n", character_player_data.talent_points_);

    if (character_player_data.player_custom_data_.initialized_)
    {
      printf("PlayerData::PlayerCustomData::Name: %ls.\n",
             GetString(process, character_player_data.player_custom_data_.name_)
               .c_str());
      DumpGameObjectNameData(
        process,
        character_player_data.player_custom_data_.name_data_,
        "PlayerData::PlayerCustomData::NameData");
    }
  }
  printf("Gender: %d\n", character.gender_);
  printf("CombatData: %p\n", character.combat_data_);
  if (character.custom_kickstarter_name_)
  {
    auto const custom_kickstarter_name =
      hadesmem::Read<StdStringW>(process, character.custom_kickstarter_name_);
    printf("CustomKickStarterName: %ls\n",
           GetString(process, custom_kickstarter_name).c_str());
  }
}
