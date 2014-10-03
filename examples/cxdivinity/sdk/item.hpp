// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include "item_template.hpp"
#include "static_assert.hpp"
#include "std_string.hpp"
#include "vec3f.hpp"

namespace divinity
{

enum ItemFlags
{
  ItemFlags_OnStage_Maybe = 0x4,
  ItemFlags_CanPickUp = 0x8,
  ItemFlags_CanMove = 0x10,
  ItemFlags_CanInteract = 0x400,
  ItemFlags_OnlyOwnerCanUse = 0x4000,
  ItemFlags_IsVisible = 0x20000,
  ItemFlags_IsStoryItem = 0x40000,
};

enum ItemFlags2
{
  ItemFlags2_UnsoldGenerated = 0x1,
  ItemFlags2_IsKey = 0x2,
  ItemFlags2_IsGlobal = 0x4,
  ItemFlags2_TreasureGenerated = 0x10,
};

struct ItemStats
{
  int field_0;
  int level_;
  int field_8[8];
  TriStringPairPoly name_data_;
  int field_AC;
  int field_B0[6];
  int field_C8;
  int field_CC[7];
  int is_identified_;
  int field_EC[4];
  int durability_;
  int durability_counter_;
  int repair_durability_penalty_;
  int field_108;
  char* item_type_;
  // int field_110[100];
};

// HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(ItemStats) == 0x110);

struct ItemData
{
  Vec3f rotate_[3];
  float scale_;
  char* uuid_;
  int field_4C;
  int flags_2_;
  int field_54;
  int field_38;
  int field_3C;
  int field_40;
  Vec3f velocity_;
  int field_70;
  ItemTemplate* current_template_;
  ItemTemplate* original_template_;
  char* stats_id_;
  ItemStats* stats_;
  int field_84;
  int field_88;
  int inventory_;
  int parent_;
  std::int16_t slot_;
  std::int16_t field_96;
  int amount_;
  int vitality_;
  int field_A0;
  char* key_;
  int lock_level_;
  float surface_check_timer_;
  float life_time_;
  void* item_machine_;
  void* plan_manager_;
  void* variable_manager_;
  void* status_manager_;
  int field_C4;
  int field_C8;
  int owner_handle_;
  int field_D0;
  int field_D4;
  int field_D8;
  int field_DC;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(ItemData) == 0xC0);

struct Item
{
  virtual ~Item() = 0;

  virtual void Unknown0004() = 0;

  virtual void Unknown0008() = 0;

  virtual unsigned int SetHandle(unsigned int handle) = 0;

  virtual unsigned int GetHandle(unsigned int* handle_out) = 0;

  virtual void SetUUID(char** uuid) = 0;

  virtual char** GetUUID() = 0;

  virtual void Unknown001C() = 0;

  virtual void Unknown0020() = 0;

  virtual Item* SetCurrentTemplate(ItemTemplate* current_template) = 0;

  virtual ItemTemplate* GetCurrentTemplate() = 0;

  virtual void Unknown002C() = 0;

  virtual bool IsGlobal() = 0;

  virtual void Unknown0034() = 0;

  virtual StdStringA* GetTemplateName() = 0;

  virtual void SetFlag(ItemFlags flag) = 0;

  virtual void ClearFlag(ItemFlags flag) = 0;

  virtual bool IsFlagSet(ItemFlags flag) = 0;

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

  virtual ItemTemplate*
    SetOriginalTemplate(ItemTemplate* original_template) = 0;

  virtual ItemTemplate* GetOriginalTemplate() = 0;

  virtual void Unknown00E0() = 0;

  Vec3f position_;
  int handle_;
  int flags_;
  int field_18;
  char* level_;
  ItemData item_data_;
};

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(Item) == 0xE0);
}
