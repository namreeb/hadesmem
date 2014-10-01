// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "ability.hpp"
#include "character_template.hpp"
#include "static_assert.hpp"
#include "status.hpp"
#include "std_string.hpp"
#include "tri_string.hpp"
#include "vec3f.hpp"

namespace divinity
{

enum CharacterFlags
{
  CharacterFlags_IsPlayer = 0x2,
  CharacterFlags_IsMultiPlayer = 0x4,
  CharacterFlags_IsPartyMember = 0x8,
  CharacterFlags_OnStage_Maybe = 0x40,
  CharacterFlags_IsDead = 0x80,
  CharacterFlags_IsSummon = 0x100,
  CharacterFlags_IsCharmed_Maybe = 0x400,
  CharacterFlags_IsInvulnerable = 0x800,
  CharacterFlags_IsImmortal = 0x1000,
  CharacterFlags_IsStoryNpc = 0x2000,
  CharacterFlags_CanFight = 0x10000,
  CharacterFlags_IsVisible = 0x20000,
  CharacterFlags_IsFloating = 0x800000,
  CharacterFlags_IsInFightMode = 0x1000000,
  CharacterFlags_CanSpotSneakers = 0x2000000,
  CharacterFlags_HasReputationEffects = 0x20000000,
};

enum CharacterFlags2
{
  CharacterFlags2_Global = 0x1,
  CharacterFlags2_HasOsirisDialog = 0x2,
  CharacterFlags2_HasDefaultDialog = 0x4,
  CharacterFlags2_TurnEnded = 0x8,
  CharacterFlags2_TreasureGeneratedForTrader = 0x20,
  CharacterFlags2_Trader = 0x40,
  CharacterFlags2_IsBoss = 0x80,
  CharacterFlags2_ScriptAllowsAoO = 0x100,
  CharacterFlags2_IsResurrected = 0x200,
  CharacterFlags2_IsPet = 0x800,
};

enum CharacterStatsFlags
{
  CharacterStatsFlags_Flanked = 0x1,
  CharacterStatsFlags_IsPlayer = 0x100,
  CharacterStatsFlags_InParty = 0x200,
  CharacterStatsFlags_EquipmentValidated = 0x2000,
};

enum CharacterCombatDataFlags
{
  CharacterCombatFlags_HasCastedSpellLastTurn = 0x1,
};

struct CharacterOwner
{
  char* unknown_str_;
  // int field_4[100];
};

// HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterOwner) == 0x194);

struct CharacterPlayerPreviousPickpocketTargetHashTableEntry
{
  CharacterPlayerPreviousPickpocketTargetHashTableEntry* next_;
  int handle_;
  int target_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(
  sizeof(CharacterPlayerPreviousPickpocketTargetHashTableEntry) == 0xC);

struct CharacterPlayerPreviousPickpocketTargetsHashTable
{
  int num_buckets_;
  CharacterPlayerPreviousPickpocketTargetHashTableEntry** table_;
  int num_entries_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(
  sizeof(CharacterPlayerPreviousPickpocketTargetsHashTable) == 0xC);

struct CharacterStatsPermanantBoost
{
  int field_0;
  int field_4;
  int field_8;
  int strength_;
  int speed_;
  int intelligence_;
  int movement_;
  int movement_speed_boost_;
  int dexterity_;
  int perception_;
  int constitution_;
  int fire_resistance_;
  int earth_resistance_;
  int water_resistance_;
  int air_resistance_;
  int poison_resistance_;
  int piercing_resistance_;
  int crushing_resistance_;
  int slashing_resistance_;
  int shadow_resistance_;
  int sight_;
  int hearing_;
  int fov_;
  int ap_maximum_;
  int ap_start_;
  int ap_recovery_;
  int critical_chance_;
  int initiative_;
  int vitality_;
  int magic_points_;
  int level_;
  int gain_;
  int armor_;
  int armor_boost_;
  int offense_;
  int defense_;
  int max_resistance_;
  int weight_;
  int chance_to_hit_boost_;
  int damage_boost_;
  int ap_cost_boost_;
  Ability abilities_[34];
  int talents_;
  int unk_maybe_talents_related_1_;
  int unk_maybe_talents_related_2_;
  __int16 traits_[18];
  int flags_;
  int boost_conditions_;
  int translation_key_;
  int has_reflection_;
  // int field_16C[100];
};

// HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterStatsPermanantBoost) ==
// 0x2FC);

struct CharacterStatsPermanantBoostPtr
{
  CharacterStatsPermanantBoost* permanant_boost_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterStatsPermanantBoostPtr) ==
                                  0x4);

struct CharacterPlayerUpgradeDataAttributesData
{
  int strength_;
  int dexterity_;
  int intelligence_;
  int constitution_;
  int speed_;
  int perception_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(
  sizeof(CharacterPlayerUpgradeDataAttributesData) == 0x18);

struct CharacterPlayerCustomData
{
  bool initialized_;
  char field_1;
  char field_2;
  char field_3;
  StdStringW name_;
  TriStringPairPoly name_data_;
  char* class_type_;
  int skin_color_;
  int hair_color_;
  int cloth_color_1_;
  int cloth_color_2_;
  int cloth_color_3_;
  bool is_male_;
  char gap_BD[3];
  TriStringPairPoly race_;
  char* icon_;
  int random_;
  char* owner_profile_id_;
  char* reserved_profile_id_;
  char* ai_personality_;
  char* speaker_;
  char* hench_man_id_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterPlayerCustomData) == 0x160);

struct CharacterStats
{
  int vtable_;
  int level_;
  int field_8[7];
  char* field_24;
  TriStringPairPoly field_28;
  int field_AC[10];
  int vitality_;
  int magic_points_;
  int action_points_;
  int bonus_action_points_;
  int experience_;
  int field_E8;
  int field_EC;
  int field_F0;
  int reputation_;
  int flags_;
  int trait_order_;
  int field_100[12];
  CharacterStatsPermanantBoostPtr* permanant_boosts_beg_;
  CharacterStatsPermanantBoostPtr* permanant_boosts_end_;
  int field_138[31];
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterStats) == 0x1B4);

struct CharacterPlayerUpgradeData
{
  CharacterStats* stats_;
  int attribute_points_;
  int ability_points_;
  int talent_points_;
  int attributes_;
  CharacterPlayerUpgradeDataAttributesData* attributes_data_;
  int field_18;
  int field_1C;
  int abilities_;
  int* abilities_data_;
  int field_28;
  int field_2C;
  int talents_;
  int unk_maybe_talents_related_1_;
  int unk_maybe_talents_related_2_;
  int field_3C;
  int unk_maybe_traits_related_1_;
  int field_44;
  int field_48;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterPlayerUpgradeData) == 0x4C);

struct CharacterPlayerData
{
  void* vtable_;
  int field_4;
  int field_8;
  int field_C;
  CharacterPlayerUpgradeData upgrade_data_;
  int field_5C;
  int field_60;
  int field_64;
  int field_68;
  int can_trade_with_;
  int field_70;
  int field_74;
  int field_78;
  int field_7C;
  int field_80;
  int field_84;
  int field_88;
  bool level_up_marker_;
  unsigned __int8 selected_skill_set_index_;
  char field_8E;
  char field_8F;
  CharacterPlayerCustomData player_custom_data_;
  CharacterPlayerPreviousPickpocketTargetsHashTable*
    previous_pickpocket_targets_;
  int field_1F4;
  int field_1F8;
  int field_1FC;
  int field_200;
  int field_204;
  int previous_position_id_;
  int helmet_option_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterPlayerData) == 0x210);

struct CharacterSkill
{
  // int field_0[100];
};

// HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterSkill) == 0x190);

struct CharacterSkillManagerHashTableEntry
{
  CharacterSkillManagerHashTableEntry* next_;
  char* name_;
  CharacterSkill* skill_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterSkillManagerHashTableEntry) ==
                                  0xC);

struct CharacterSkillManagerHashTable
{
  int num_buckets_;
  CharacterSkillManagerHashTableEntry** table_;
  int num_entries_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterSkillManagerHashTable) ==
                                  0xC);

struct CharacterSkillManager
{
  void* field_0;
  int unk_character_handle_;
  CharacterSkillManagerHashTable skills_;
  // int field_14[100];
};

// HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterSkillManager) == 0x1A4);

struct CharacterCombatData
{
  int field_0;
  int field_4;
  int field_8;
  CharacterCombatDataFlags flags_;
  int status_history_;
  // int field_14[100];
};

// HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterCombatData) == 0x1A4);

struct CharacterAlignmentData
{
  int field_0;
  int field_4;
  int field_8;
  int field_C;
  char* alignment_;
  // int field_14[50];
};

// HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterAlignmentData) == 0xDC);

struct CharacterData
{
  Vec3f rotate_[3];
  float scale_;
  char* uuid_;
  int field_4C;
  int field_50;
  int field_54;
  int field_58;
  int field_5C;
  CharacterTemplate* current_template_;
  CharacterTemplate* original_template_;
  int flags_2_;
  float field_6C;
  float field_70;
  float field_74;
  int field_78;
  CharacterAlignmentData* alignment_data_;
  char* animation_override_;
  bool needs_update_;
  bool script_force_update_;
  char field_86;
  char field_87;
  CharacterStats* stats_;
  unsigned int inventory_handle_;
  void* movement_machine_;
  int field_94;
  void* steering_machine_;
  int field_9C;
  int field_A0;
  int field_A4;
  void* osiris_controller_;
  int field_AC;
  int field_B0;
  int field_B4;
  int field_B8;
  void* dialog_controller_;
  GameObjectStatusManager* status_manager_;
  CharacterSkillManager* skill_manager_;
  void* variable_manager_;
  int field_CC;
  int field_D0;
  int field_D4;
  CharacterOwner* owner_;
  int field_DC;
  __int16 team_id_;
  __int16 field_E2;
  float life_time_;
  float trigger_traps_timer_;
  float surface_distance_check_;
  float surface_timer_check_;
  int previous_surface_tile_state_;
  int field_F8;
  int reserved_peer_id_;
  unsigned int owner_handle_;
  unsigned int follow_handle_;
  unsigned int enemy_handle_;
  int enemies_;
  int field_110;
  int field_114;
  int hostile_count_;
  void* plan_manager_;
  float partial_ap_;
  bool current_aoo_;
  char field_125;
  char field_126;
  char field_127;
  int registered_trigger_;
  int field_12C;
  int field_130;
  int field_134;
  CharacterPlayerData* player_data_;
  int field_13C;
  float field_140;
  float field_144;
  int gender_;
  CharacterCombatData* combat_data_;
  StdStringW* custom_kickstarter_name_;
  int field_154;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(CharacterData) == 0x138);

struct Character
{
  virtual ~Character() = 0;

  virtual void Unknown0004() = 0;

  virtual void Unknown0008() = 0;

  virtual unsigned int* SetHandle(unsigned int a2) = 0;

  virtual unsigned int* GetHandle(unsigned int* a2) = 0;

  virtual void SetUUID(char** a2) = 0;

  virtual char** GetUUID() = 0;

  virtual void Unknown001C() = 0;

  virtual void Unknown0020() = 0;

  virtual Character*
    SetCurrentTemplate(CharacterTemplate* current_template) = 0;

  virtual CharacterTemplate* GetCurrentTemplate() = 0;

  virtual void Unknown002C() = 0;

  virtual bool IsGlobal() = 0;

  virtual void Unknown0034() = 0;

  virtual StdStringA* GetTemplateName() = 0;

  virtual void SetFlag(CharacterFlags flag) = 0;

  virtual void ClearFlag(CharacterFlags flag) = 0;

  virtual bool IsFlagSet(CharacterFlags flag) = 0;

  virtual Vec3f* GetPosition() = 0;

  virtual Vec3f* GetRotation() = 0;

  virtual void Unknown0050() = 0;

  virtual void Unknown0054() = 0;

  virtual void Unknown0058() = 0;

  virtual void Unknown005C() = 0;

  virtual void Unknown0060() = 0;

  virtual void Unknown0064() = 0;

  virtual void Unknown0068() = 0;

  virtual void Unknown006C() = 0;

  virtual void Unknown0070() = 0;

  virtual void Unknown0074() = 0;

  virtual void Unknown0078() = 0;

  virtual void Unknown007C() = 0;

  virtual void Unknown0080() = 0;

  virtual void Unknown0084() = 0;

  virtual void Unknown0088() = 0;

  virtual void Unknown008C() = 0;

  virtual void Unknown0090() = 0;

  virtual void Unknown0094() = 0;

  virtual void Unknown0098() = 0;

  virtual void Unknown009C() = 0;

  virtual void Unknown00A0() = 0;

  virtual void Unknown00A4() = 0;

  virtual void Unknown00A8() = 0;

  virtual void Unknown00AC() = 0;

  virtual void Unknown00B0() = 0;

  virtual void Unknown00B4() = 0;

  virtual void Unknown00B8() = 0;

  virtual void Unknown00BC() = 0;

  virtual void Unknown00C0() = 0;

  virtual void Unknown00C4() = 0;

  virtual void Unknown00C8() = 0;

  virtual void Unknown00CC() = 0;

  virtual TriStringPairPoly* GetNameData(TriStringPairPoly* name_out);

  virtual void Unknown00D4() = 0;

  virtual CharacterTemplate*
    SetOriginalTemplate(CharacterTemplate* original_template) = 0;

  virtual CharacterTemplate* GetOriginalTemplate() = 0;

  virtual void Unknown00E0() = 0;

  Vec3f position_;
  unsigned int handle_;
  int flags_;
  float field_18;
  char* level_;
  CharacterData character_data_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(Character) == 0x158);
}
