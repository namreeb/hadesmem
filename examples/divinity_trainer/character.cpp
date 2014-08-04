// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "character.hpp"

#include <cstdio>

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
         !!(character.flags_ & CharacterFlags::CharacterFlags_IsPlayer));
  printf("Flags::IsPartyMember: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_IsPartyMember));
  printf("Flags::IsDead: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_IsDead));
  printf("Flags::IsSummon: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_IsSummon));
  printf("Flags::IsInvulnerable: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_IsInvulnerable));
  printf("Flags::IsImmortal: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_IsImmortal));
  printf("Flags::IsStoryNpc: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_IsStoryNpc));
  printf("Flags::CanFight: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_CanFight));
  printf("Flags::IsInvisible: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_IsVisible));
  printf("Flags::IsFloating: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_IsFloating));
  printf("Flags::IsInFightMode: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_IsInFightMode));
  printf("Flags::CanSpotSneakers: %d\n",
         !!(character.flags_ & CharacterFlags::CharacterFlags_CanSpotSneakers));
  printf(
    "Flags::HasReputationEffects: %d\n",
    !!(character.flags_ & CharacterFlags::CharacterFlags_HasReputationEffects));
  printf("Region: %s\n",
         hadesmem::ReadString<char>(process, character.region_).c_str());
  printf("UUID: %s\n",
         hadesmem::ReadString<char>(process, character.uuid_).c_str());
  printf("CurrentTemplate: %p\n",
         static_cast<void*>(character.current_template_));
  auto const current_character_template =
    hadesmem::Read<CharacterTemplate>(process, character.current_template_);
  printf("CurrentTemplate::Name: %s\n",
         GetString(process, current_character_template.name_).c_str());
  printf("OriginalTemplate: %p\n",
         static_cast<void*>(character.original_template_));
  auto const original_character_template =
    hadesmem::Read<CharacterTemplate>(process, character.current_template_);
  printf("OriginalTemplate::Name: %s\n",
         GetString(process, original_character_template.name_).c_str());
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
  printf("IsInCombat: %d\n", character.is_in_combat_);
  printf("ReservedPeerId: %d\n", character.reserved_peer_id_);
  printf("PlayerData: %p\n", static_cast<void*>(character.player_data_));
  if (character.player_data_)
  {
    auto const character_player_data =
      hadesmem::Read<CharacterPlayerData>(process, character.player_data_);
    printf("PlayerData::Attribute: %d\n",
           character_player_data.attribute_points_);
    printf("PlayerData::Ability: %d\n", character_player_data.ability_points_);
    printf("PlayerData::Talent: %d\n", character_player_data.talent_points_);
  }
  printf("Gender: %d\n", character.gender_);
}
