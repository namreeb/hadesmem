// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "flagged.hpp"
#include "static_assert.hpp"
#include "std_string.hpp"

namespace divinity
{

struct CharacterTemplateTreasures
{
  char** cstrs_;
  int count_1_;
  int count_2_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterTemplateTreasures) == 0xC);

struct CharacterTemplateGroups
{
  StdStringA* strs_;
  int count_1_;
  int count_2_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterTemplateGroups) == 0xC);

struct CharacterTemplate
{
  void* vtable_;
  int field_4;
  int field_8;
  char* map_key_;
  StdStringA name_;
  char* template_name_;
  int field_30;
  bool is_global_;
  char field_35;
  char field_36;
  char field_37;
  char* level_name_;
  int field_3C;
  int field_40[34];
  char* visual_template_;
  int field_CC;
  char* physics_template_;
  int field_D4;
  FlaggedBool cast_shadow_;
  FlaggedBool recieve_decal_;
  FlaggedBool is_reflecting_;
  char field_DE;
  char field_DF;
  int field_E0;
  int field_E4;
  bool mod_removed_;
  char field_E9;
  bool has_parent_mod_relation_;
  char field_EB;
  int field_EC[28];
  TriStringPairPoly name_data_;
  int field_1E0[7];
  FlaggedCString icon_;
  FlaggedCString alignment_;
  FlaggedCString stats_;
  FlaggedCString skill_set_;
  FlaggedCString equipment_;
  int field_224;
  CharacterTemplateTreasures treasures_;
  int field_234;
  FlaggedCString light_id_;
  FlaggedCString hit_fx_;
  FlaggedCString default_dialog_;
  FlaggedFloat walk_speed_;
  FlaggedFloat run_speed_;
  FlaggedBool can_fight_;
  FlaggedBool can_see_through_;
  FlaggedBool can_shoot_through_;
  FlaggedBool is_player_;
  FlaggedBool floating_;
  FlaggedBool spot_sneakers_;
  FlaggedBool is_boss_;
  FlaggedBool can_open_doors_;
  FlaggedBool avoid_traps_;
  FlaggedBool influence_treasure_level_;
  FlaggedDword equipment_class_;
  int field_27C;
  CharacterTemplateGroups groups_;
  int field_28C;
  int on_death_actions_;
  int field_294[4];
  FlaggedDword blood_surface_type_;
  int field_2AC;
  int field_2B0;
  int field_2B4;
  int field_2B8;
  int field_2BC;
  int field_2C0;
  int field_2C4;
  FlaggedDword random_;
  FlaggedCString trophy_id_;
  FlaggedCString sound_switch_;
  FlaggedCString combat_group_id_;
  FlaggedDword level_override_;
  FlaggedDword gender_;
  FlaggedCString speaker_;
  FlaggedCString alt_speaker_;
  FlaggedCString speaker_greeting_override_;
  FlaggedBool force_unsheath_skills_;
  FlaggedBool can_be_teleported_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterTemplate) == 0x314);
}
