// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/config.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/process.hpp>

#include "hash_table.hpp"
#include "std_string.hpp"
#include "tri_string.hpp"
#include "vec3f.hpp"

enum CharacterFlags
{
  CharacterFlags_IsPlayer = 0x2,
  CharacterFlags_IsMultiPlayer = 0x4,
  CharacterFlags_IsPartyMember = 0x8,
  CharacterFlags_IsDead = 0x80,
  CharacterFlags_IsSummon = 0x100,
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

struct FlaggedCString
{
  char *cstr_;
  char initialized_;
  char field_5;
  char field_6;
  char field_7;
};

struct FlaggedBool
{
  bool bool_;
  bool initialized_;
};

struct FlaggedFloat
{
  float float_;
  bool initialized_;
  char field_5;
  char field_6;
  char field_7;
};

struct FlaggedDword
{
  int dword_;
  char initialized_;
  char field_5;
  char field_6;
  char field_7;
};

struct CharacterTemplate
{
  int vtable_;
  int field_4;
  int field_8;
  char *map_key_;
  StdStringA name_;
  char *template_name_;
  int field_30;
  bool is_global_;
  char field_35;
  char field_36;
  char field_37;
  char *level_name_;
  int field_3C;
  int field_40[34];
  char *visual_template_;
  int field_CC;
  char *physics_template_;
  int field_D4;
  bool cast_shadow_[2];
  bool recieve_decal_[2];
  bool is_reflecting_[2];
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
  int field_228;
  int field_22C;
  int field_230;
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
  StdStringA *unk_str_narrow_array_ptr_;
  int field_284;
  int unk_str_narrow_array_count_;
  int field_28C;
  int on_death_actions_;
  int field_294[4];
  FlaggedDword blood_surface_type_;
  int field_2AC[9];
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
  //int field_314[100];
};

struct CharacterStats
{
  virtual ~CharacterStats() {}

  int level_;
  int field_8[7];
  char *field_24;
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
  int field_100[9];
  int field_124;
  int field_128;
  int field_12C;
  int field_130;
  int field_134;
  int field_138[16];
  int field_178;
  int field_17C;
  int field_180;
  int field_184[100];
};

struct CharacterPlayerCustomData
{
  bool initialized_;
  char field_1;
  char field_2;
  char field_3;
  StdStringW name_;
  TriStringPairPoly name_data_;
  char *class_type_;
  int skin_color_;
  int hair_color_;
  int cloth_color_1_;
  int cloth_color_2_;
  int cloth_color_3_;
  int is_male_;
  int race_;
  int field_C4[32];
  char *icon_;
  int random_;
  char *owner_profile_id_;
  char *reserved_profile_id_;
  char *ai_personality_;
  char *speaker_;
  char *hench_man_id_;
};

struct CharacterAlignmentData
{
  int field_0;
  int field_4;
  int field_8;
  int field_C;
  char *alignment_;
};

struct CharacterPlayerData
{
  int field_0[5];
  int attribute_points_;
  int ability_points_;
  int talent_points_;
  int field_20[19];
  int can_trade_with_;
  int field_70[7];
  char level_up_marker_;
  char selected_skill_set_index_;
  char field_8E;
  char field_8F;
  CharacterPlayerCustomData player_custom_data_;
  int field_1F0[6];
  int previous_position_id_;
  char helmet_option_;
  char field_20D;
  char field_20E;
  char field_20F;
  //int field_210[50];
};

struct CharacterCombatData
{
  int field_0[3];
  int unk_flags_;
  int unk_status_array_[100];
};

struct CharacterStatus
{
  virtual ~CharacterStatus() {}

  int field_4[11];
  CharacterAlignmentData *charmed_alignment_data_;
  int field_34[100];
};

struct CharacterStatusManagerCharacterStatusVector
{
  CharacterStatus **statuses_;
  unsigned int capacity_;
  unsigned int size_;
};

struct CharacterStatusManager
{
  void *vtable_;
  int field_4[39];
  CharacterStatusManagerCharacterStatusVector statuses_;
  //int field_AC[100];
};

struct CharacterSkill
{
  int field_0[100];
};

struct CharacterSkillManager
{
  int field_0;
  int unk_character_handle_;
  HashTable<char*, CharacterSkill*> skills_;
  int field_10[100];
};

struct Character
{
  virtual ~Character()
  {
  }

  virtual void Unknown0004()
  {
  }

  virtual void Unknown0008()
  {
  }

  virtual int* SetHandle(std::uint32_t /*handle*/)
  {
    return nullptr;
  }

  virtual std::uint32_t* GetHandle(std::uint32_t* /*handle*/)
  {
    return nullptr;
  }

  virtual void SetUUID(char** /*uuid*/)
  {
  }

  virtual char** GetUUID()
  {
    return nullptr;
  }

  virtual void Unknown001C()
  {
  }

  virtual void Unknown0020()
  {
  }

  virtual CharacterTemplate*
    SetCurrentTemplate(CharacterTemplate* /*current_template*/)
  {
    return nullptr;
  }

  virtual CharacterTemplate* GetCurrentTemplate()
  {
    return nullptr;
  }

  virtual void Unknown002C()
  {
  }

  virtual void Unknown0030()
  {
  }

  virtual void Unknown0034()
  {
  }

  virtual StdStringA* GetTemplateName()
  {
    return nullptr;
  }

  virtual void SetFlag(CharacterFlags /*flag*/)
  {
  }

  virtual void ClearFlag(CharacterFlags /*flag*/)
  {
  }

  virtual bool IsFlagSet(CharacterFlags /*flag*/)
  {
    return false;
  }

  virtual Vec3f* GetPosition()
  {
    return nullptr;
  }

  virtual void Unknown004C()
  {
  }

  virtual void Unknown0050()
  {
  }

  virtual void Unknown0054()
  {
  }

  virtual void Unknown0058()
  {
  }

  virtual void Unknown005C()
  {
  }

  virtual void Unknown0060()
  {
  }

  virtual void Unknown0064()
  {
  }

  virtual void Unknown0068()
  {
  }

  virtual void Unknown006C()
  {
  }

  virtual void Unknown0070()
  {
  }

  virtual void Unknown0074()
  {
  }

  virtual void Unknown0078()
  {
  }

  virtual void Unknown007C()
  {
  }

  virtual void Unknown0080()
  {
  }

  virtual void Unknown0084()
  {
  }

  virtual void Unknown0088()
  {
  }

  virtual void Unknown008C()
  {
  }

  virtual void Unknown0090()
  {
  }

  virtual void Unknown0094()
  {
  }

  virtual void Unknown0098()
  {
  }

  virtual void Unknown009C()
  {
  }

  virtual void Unknown00A0()
  {
  }

  virtual void Unknown00A4()
  {
  }

  virtual void Unknown00A8()
  {
  }

  virtual void Unknown00AC()
  {
  }

  virtual void Unknown00B0()
  {
  }

  virtual void Unknown00B4()
  {
  }

  virtual void Unknown00BC()
  {
  }

  virtual void Unknown00C0()
  {
  }

  virtual void Unknown00C4()
  {
  }

  virtual void Unknown00C8()
  {
  }

  virtual void Unknown00CC()
  {
  }

  virtual void Unknown00D0()
  {
  }

  virtual void Unknown00D4()
  {
  }

  virtual CharacterTemplate*
    SetOriginalTemplate(CharacterTemplate* /*character_template*/)
  {
    return nullptr;
  }

  virtual CharacterTemplate* GetOriginalTemplate()
  {
    return nullptr;
  }

  virtual void Unknown00E0()
  {
  }

  Vec3f position_;
  int handle_;
  int flags_;
  float field_18;
  char *level_;
  Vec3f rotate_[3];
  float scale_;
  char *uuid_;
  int field_4C;
  int field_50;
  int field_54;
  int field_58;
  int field_5C;
  CharacterTemplate *current_template_;
  CharacterTemplate *original_template_;
  CharacterFlags2 flags_2_;
  int field_6C;
  int field_70;
  int field_74;
  int field_78;
  CharacterAlignmentData *alignment_data_;
  char *animation_override_;
  char needs_update_;
  char script_force_update_;
  char field_86;
  char field_87;
  CharacterStats *stats_;
  int inventory_handle_;
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
  CharacterStatusManager *status_manager_;
  CharacterSkillManager *skill_manager_;
  void* variable_manager_;
  int field_CC;
  int field_D0;
  int field_D4;
  int owner_;
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
  int owner_handle_;
  int follow_handle_;
  int enemy_handle_;
  int enemies_;
  int field_110;
  int field_114;
  int hostile_count_;
  void* plan_manager_;
  int partial_ap_;
  int current_aoo_;
  int registered_trigger_;
  int field_12C;
  int field_130;
  int field_134;
  CharacterPlayerData *player_data_;
  int field_13C;
  int field_140;
  int field_144;
  int gender_;
  CharacterCombatData *combat_data_;
  StdStringW *custom_kickstarter_name_;
  //int field_154[50];
};

#if defined(HADESMEM_DETAIL_ARCH_X86)
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(Character) == 0x154);
#endif // #if defined(HADESMEM_DETAIL_ARCH_X86)

void DumpCharacter(hadesmem::Process const& process, Character* character_ptr);
