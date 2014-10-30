// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include <hadesmem/detail/static_assert_x86.hpp>

#include "std_string.hpp"
#include "vec3f.hpp"

namespace divinity
{
struct CharacterAlignmentData;

enum Status
{
  Status_NONE = 0x0,
  Status_HIT = 0x1,
  Status_DYING = 0x2,
  Status_HEAL = 0x3,
  Status_BURNING = 0x4,
  Status_FROZEN = 0x5,
  Status_POISONED = 0x6,
  Status_STUNNED = 0x7,
  Status_WET = 0x8,
  Status_MUTED = 0x9,
  Status_CHARMED = 0xA,
  Status_KNOCKED_DOWN = 0xB,
  Status_SUMMONING = 0xC,
  Status_HEALING = 0xD,
  Status_THROWN = 0xE,
  Status_SHIELD = 0xF,
  Status_FALLING = 0x10,
  Status_CONSUME = 0x11,
  Status_COMBAT = 0x12,
  Status_ATTACKOFOPP = 0x13,
  Status_STORY_FROZEN = 0x14,
  Status_SNEAKING = 0x15,
  Status_UNLOCK = 0x16,
  Status_FEAR = 0x17,
  Status_BLACKROCKED = 0x18,
  Status_BOOST = 0x19,
  Status_UNSHEATHED = 0x1A,
  Status_STANCE = 0x1B,
  Status_SITTING = 0x1C,
  Status_LYING = 0x1D,
  Status_CRIPPLED = 0x1E,
  Status_CURSED = 0x1F,
  Status_WEAK = 0x20,
  Status_SLOWED = 0x21,
  Status_HASTED = 0x22,
  Status_RAGED = 0x23,
  Status_LUCKY = 0x24,
  Status_DISEASED = 0x25,
  Status_FORTIFIED = 0x26,
  Status_BLESSED = 0x27,
  Status_DRUNK = 0x28,
  Status_CHILLED = 0x29,
  Status_WARM = 0x2A,
  Status_BLEEDING = 0x2B,
  Status_BLIND = 0x2C,
  Status_SMELLY = 0x2D,
  Status_CLEAN = 0x2E,
  Status_INFECTIOUS_DISEASED = 0x2F,
  Status_PETRIFIED = 0x30,
  Status_INVISIBLE = 0x31,
  Status_ROTATE = 0x32,
  Status_ENCUMBERED = 0x33,
  Status_IDENTIFY = 0x34,
  Status_REPAIR = 0x35,
  Status_INVULNERABLE = 0x36,
  Status_VOID_AURA = 0x37,
  Status_MATERIAL = 0x38,
  Status_LEADERSHIP = 0x39,
  Status_InvalidIdMaxValue = 0x3A,
};

enum StatusFlags
{
  StatusFlags_ForceStatus = 0x1,
  StatusFlags_RequestDelete = 0x8,
  StatusFlags_Active = 0x10,
  StatusFlags_KeepAlive = 0x20,
  StatusFlags_IsFromItem = 0x80,
};

struct GameObjectStatus
{
  virtual ~GameObjectStatus() = 0;

  virtual void SetUnknownHandle(unsigned int a2) = 0;

  virtual unsigned int* GetUnknownHandle(unsigned int* a2) = 0;

  virtual Status GetStatusType() = 0;

  virtual void Unknown0010() = 0;

  virtual void Unknown0014() = 0;

  virtual char** GetStatsId() = 0;

  virtual void Unknown001C() = 0;

  virtual void Unknown0020() = 0;

  virtual void Unknown0024() = 0;

  virtual void Unknown0028() = 0;

  virtual void Unknown002C() = 0;

  virtual void Unknown0030() = 0;

  virtual void Unknown0034() = 0;

  virtual void Unknown0038() = 0;

  virtual void Unknown003C() = 0;

  virtual void Unknown0040() = 0;

  virtual void Unknown0044() = 0;

  virtual void Unknown0048() = 0;

  virtual void Unknown004C() = 0;

  virtual void Unknown0050() = 0;

  virtual void Unknown0054() = 0;

  virtual void Unknown0058() = 0;

  virtual void Unknown005C() = 0;

  virtual void Unknown0060() = 0;

  virtual void Unknown0064() = 0;

  virtual void Unknown0068() = 0;

  char* field_4;
  std::int16_t field_8;
  std::int16_t field_A;
  int can_enter_chance_;
  float activate_timer_;
  float life_time_;
  float current_life_time_;
  int status_cause_;
  unsigned int unk_handle_1_;
  int parent_;
  int status_source_;
  std::int16_t team_id_;
  char flags_;
  char field_2F;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatus) == 0x30);

struct GameObjectStatusManagerGameObjectVector
{
  GameObjectStatus** statuses_;
  unsigned int capacity_;
  unsigned int size_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(
  sizeof(GameObjectStatusManagerGameObjectVector) == 0xC);

struct GameObjectStatusManager
{
  void* vtable_;
  int field_4[20];
  GameObjectStatusManagerGameObjectVector unknown_statuses_;
  int field_60[16];
  GameObjectStatusManagerGameObjectVector statuses_;
  int unknown_handle_maybe_parent_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusManager) == 0xB0);

struct GameObjectStatusHitDamageData
{
  int equipment_;
  int total_damage_done_;
  void* field_8;
  int field_C;
  int field_10;
  void* field_14;
  int field_18;
  int field_1C;
  int death_type_;
  int armor_absorption_;
  std::int16_t effect_flags_;
  std::int16_t field_2A;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusHitDamageData) ==
                                  0x2C);

struct GameObjectStatusHit : public GameObjectStatus
{
  float field_30;
  int field_34;
  GameObjectStatusHitDamageData damage_data_;
  int hit_by_handle_;
  int hit_with_handle_;
  int weapon_handle_;
  int hit_reason_;
  char* skill_id_;
  char field_78;
  char field_79;
  char field_7A;
  char field_7B;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusHit) == 0x7C);

struct GameObjectStatusDying : public GameObjectStatus
{
  int unknown_handle_;
  int source_type_;
  int death_type_;
  bool is_already_dead_;
  bool die_actions_completed_;
  char field_3E;
  char field_3F;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusDying) == 0x40);

struct GameObjectStatusHeal : public GameObjectStatus
{
  float effect_time_;
  int heal_amount_;
  int heal_effect_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusHeal) == 0x3C);

struct GameObjectStatusBurning : public GameObjectStatus
{
  int field_30;
  int field_34;
  int field_38;
  int field_3C;
  int field_40;
  float hit_timer_;
  float time_elapsed_;
  int field_4C;
  int damage_level_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusBurning) == 0x54);

struct GameObjectStatusFrozen : public GameObjectStatus
{
  int field_30;
  int field_34;
  int field_38;
  int field_3C;
  int field_40;
  Vec3f freeze_direction_;
  float field_50;
  float field_54;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusFrozen) == 0x58);

struct GameObjectStatusPoisoned : public GameObjectStatus
{
  float hit_timer_;
  float time_elapsed_;
  int damage_level_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusPoisoned) == 0x3C);

struct GameObjectStatusStunned : public GameObjectStatus
{
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusStunned) == 0x30);

struct GameObjectStatusWet : public GameObjectStatus
{
  int field_30;
  int field_34;
  int field_38;
  int field_3C;
  int field_40;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusWet) == 0x44);

struct GameObjectStatusCharmed : public GameObjectStatus
{
  CharacterAlignmentData* alignment_data_;
  int field_34;
  unsigned int original_owner_character_handle_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusCharmed) == 0x3C);

struct GameObjectStatusMuted : public GameObjectStatus
{
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusMuted) == 0x30);

struct GameObjectStatusKnockedDown : public GameObjectStatus
{
  int knocked_down_state_;
  char is_instant_;
  char field_35;
  char field_36;
  char field_37;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusKnockedDown) == 0x38);

struct GameObjectStatusSummoning : public GameObjectStatus
{
  float field_30;
  int level_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusSummoning) == 0x38);

struct GameObjectStatusHealing : public GameObjectStatus
{
  float field_30;
  int heal_amount_;
  float time_elapsed_;
  int heal_effect_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusHealing) == 0x40);

struct GameObjectStatusThrown : public GameObjectStatus
{
  float in_air_duration_;
  float total_duration_timer_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusThrown) == 0x38);

struct GameObjectStatusShield : public GameObjectStatus
{
  int unknown_handle_;
  char* skill_id_;
  int field_38;
  int field_3C;
  int field_40;
  int field_44;
  int field_48;
  float shield_hp_;
  float max_shield_hp_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusShield) == 0x54);

struct GameObjectStatusFalling : public GameObjectStatus
{
  Vec3f target_;
  float reappear_time_;
  char* skill_id_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusFalling) == 0x44);

struct GameObjectStatusConsume : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  char* field_38;
  int field_3C;
  int saving_throw_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusConsume) == 0x44);

struct GameObjectStatusCombat : public GameObjectStatus
{
  bool ready_for_combat_;
  char field_31;
  char field_32;
  char field_33;
  float field_34;
  std::int16_t field_38;
  std::int16_t field_3A;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusCombat) == 0x3C);

struct GameObjectStatusStoryFrozen : public GameObjectStatus
{
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusStoryFrozen) == 0x30);

struct GameObjectStatusAttackOfOpp : public GameObjectStatus
{
  unsigned int source_handle_;
  unsigned int source_target_;
  unsigned int source_partner_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusAttackOfOpp) == 0x3C);

struct GameObjectStatusSneaking : public GameObjectStatus
{
  bool client_request_stop_;
  char field_31;
  char field_32;
  char field_33;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusSneaking) == 0x34);

struct GameObjectStatusUnlock : public GameObjectStatus
{
  char* field_30;
  int level_;
  int unlocked_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusUnlock) == 0x3C);

struct GameObjectStatusFear : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusFear) == 0x44);

struct GameObjectStatusBlackRocked : public GameObjectStatus
{
  int damage_;
  float hit_timer_;
  float time_elapsed_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusBlackRocked) == 0x3C);

struct GameObjectStatusBoost : public GameObjectStatus
{
  float effect_time_;
  StdStringA boost_id_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusBoost) == 0x50);

struct GameObjectStatusUnsheathed : public GameObjectStatus
{
  bool force_;
  char field_31;
  char field_32;
  char field_33;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusUnsheathed) == 0x34);

struct GameObjectStatusStance : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  char* field_3C;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusStance) == 0x40);

struct GameObjectStatusSitting : public GameObjectStatus
{
  unsigned int item_handle_;
  int index_;
  float field_38;
  float time_elapsed_;
  float heal_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusSitting) == 0x44);

struct GameObjectStatusLying : public GameObjectStatus
{
  unsigned int item_handle_;
  int index_;
  float field_38;
  float time_elapsed_;
  float heal_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusLying) == 0x44);

struct GameObjectStatusCrippled : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusCrippled) == 0x44);

struct GameObjectStatusCursed : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusCursed) == 0x44);

struct GameObjectStatusWeak : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusWeak) == 0x44);

struct GameObjectStatusSlowed : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  char saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusSlowed) == 0x44);

struct GameObjectStatusHasted : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusHasted) == 0x44);

struct GameObjectStatusRaged : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusRaged) == 0x44);

struct GameObjectStatusLucky : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  char saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusLucky) == 0x44);

struct GameObjectStatusDiseased : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusDiseased) == 0x44);

struct GameObjectStatusFortified : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusFortified) == 0x44);

struct GameObjectStatusBlessed : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  char saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusBlessed) == 0x44);

struct GameObjectStatusChilled : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusChilled) == 0x44);

struct GameObjectStatusWarm : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusWarm) == 0x44);

struct GameObjectStatusBleeding : public GameObjectStatus
{
  float hit_timer_;
  float time_elapsed_;
  int damage_level_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusBleeding) == 0x3C);

struct GameObjectStatusBlind : public GameObjectStatus
{
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusBlind) == 0x30);

struct GameObjectStatusSmelly : public GameObjectStatus
{
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusSmelly) == 0x30);

struct GameObjectStatusClean : public GameObjectStatus
{
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusClean) == 0x30);

struct GameObjectStatusInfectiousDisease : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
  int field_44;
  int infections_;
  float infect_timer_;
  float field_50;
  unsigned int target_handle_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusInfectiousDisease) ==
                                  0x58);

struct GameObjectStatusPetrified : public GameObjectStatus
{
  float field_30;
  float field_34;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusPetrified) == 0x38);

struct GameObjectStatusInvisible : public GameObjectStatus
{
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusInvisible) == 0x30);

struct GameObjectStatusRotate : public GameObjectStatus
{
  float yaw_;
  float rotation_speed_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusRotate) == 0x38);

struct GameObjectStatusEncumbered : public GameObjectStatus
{
  int field_30;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusEncumbered) == 0x34);

struct GameObjectStatusIdentify : public GameObjectStatus
{
  int field_30;
  int field_34;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusIdentify) == 0x38);

struct GameObjectStatusRepair : public GameObjectStatus
{
  int level_;
  int repaired_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusRepair) == 0x38);

struct GameObjectStatusInvulnerable : public GameObjectStatus
{
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusInvulnerable) == 0x30);

struct GameObjectStatusVoidAura : public GameObjectStatus
{
  int field_30;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusVoidAura) == 0x34);

struct GameObjectStatusMaterial : public GameObjectStatus
{
  char* material_uuid_;
  std::uint8_t apply_flags_;
  bool is_overlay_material_;
  bool fading_;
  bool apply_normal_map_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusMaterial) == 0x38);

struct GameObjectStatusLeadership : public GameObjectStatus
{
  float effect_time_;
  char* stats_id_;
  int field_38;
  int field_3C;
  bool saving_throw_;
  char field_41;
  char field_42;
  char field_43;
  int field_44;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(GameObjectStatusLeadership) == 0x48);
}
