// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/config.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/process.hpp>

#include "std_string.hpp"
#include "vec3f.hpp"

enum CharacterFlags
{
  CharacterFlags_IsPlayer = 0x2,
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

struct CharacterTemplate
{
  int field_0;
  int field_4;
  int field_8;
  char* uuid_;
  StdStringA name_;
  int field_2C;
  int field_30;
  //int field_34[500];
};

struct CharacterStats
{
  int field_0;
  int level_;
  int field_8[51];
  int vitality_;
  int magic_points_;
  int action_points_;
  int bonus_action_points_;
  int experience_;
  int field_E8;
  int field_EC;
  int field_F0;
  int reputation_;
  int field_F8;
  int trait_order_;
  int field_100[150];
};

struct CharacterPlayerData
{
  int field_0[5];
  int attribute_points_;
  int ability_points_;
  int talent_points_;
  int field_20[200];
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
  unsigned int handle_;
  CharacterFlags flags_;
  int field_18;
  char* region_;
  int field_20[9];
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
  int field_6C;
  int field_70;
  int field_74;
  int field_78;
  int field_7C;
  int animation_override_;
  char needs_update_;
  char script_force_update_;
  char field_86;
  char field_87;
  CharacterStats* stats_;
  int inventory_handle_;
  int movement_machine_;
  int field_94;
  int steering_machine_;
  int field_9C;
  int field_A0;
  int field_A4;
  int field_A8;
  int field_AC;
  int field_B0;
  int field_B4;
  int field_B8;
  int field_BC;
  int status_manager_;
  int skill_manager_;
  int variable_manager_;
  int field_CC;
  int field_D0;
  int field_D4;
  void* owner_;
  int field_DC;
  char field_E0;
  char is_in_combat_;
  char field_E2;
  char field_E3;
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
  int plan_manager_;
  int partial_ap_;
  int current_aoo_;
  int registered_trigger_;
  int field_12C;
  int field_130;
  int field_134;
  CharacterPlayerData* player_data_;
  int field_13C;
  int field_140;
  int field_144;
  int gender_;
  int combat_data_;
  StdStringW* custom_kickstarter_name_;
  int field_154[50];
};

#if defined(HADESMEM_DETAIL_ARCH_X86)
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(Character) == 0x21C);
#endif // #if defined(HADESMEM_DETAIL_ARCH_X86)

void DumpCharacter(hadesmem::Process const& process, Character* character_ptr);
