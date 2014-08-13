// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/config.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/process.hpp>

#include "std_string.hpp"
#include "vec3f.hpp"

enum ItemFlags
{
  ItemFlags_IsVisible = 0x20000,
};

struct ItemStats
{
  int field_0;
  int level_;
  int field_8[56];
  int is_identified_;
  int field_EC[5];
  int durability_;
  int durability_counter_;
  int repair_durability_penalty_;
  int field_10C;
  char* item_type_;
  int field_114[100];
};

struct ItemTemplate
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

struct Item
{
  virtual ~Item()
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

  virtual ItemTemplate* SetCurrentTemplate(ItemTemplate* /*item_template*/)
  {
    return nullptr;
  }

  virtual ItemTemplate* GetCurrentTemplate()
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

  virtual void SetFlag(ItemFlags /*flag*/)
  {
  }

  virtual void ClearFlag(ItemFlags /*flag*/)
  {
  }

  virtual bool IsFlagSet(ItemFlags /*flag*/)
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

  virtual ItemTemplate* SetOriginalTemplate(ItemTemplate* /*item_template*/)
  {
    return nullptr;
  }

  virtual ItemTemplate* GetOriginalTemplate()
  {
    return nullptr;
  }

  virtual void Unknown00E0()
  {
  }

  Vec3f position_;
  unsigned int handle_;
  ItemFlags flags_;
  int field_18;
  char* level_;
  int field_20[9];
  float scale_;
  char* uuid_;
  int field_4C;
  int flags_2_;
  int field_54[4];
  Vec3f velocity_;
  int field_70;
  ItemTemplate* current_template_;
  ItemTemplate* original_template_;
  int field_7C;
  ItemStats* stats_;
  int field_84;
  int field_88;
  unsigned int inventory_;
  unsigned int parent_;
  short slot_;
  short field_96;
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
  unsigned int owner_;
  int field_D0;
  int field_D4[100];
};

#if defined(HADESMEM_DETAIL_ARCH_X86)
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(Item) == 0x264);
#endif // #if defined(HADESMEM_DETAIL_ARCH_X86)

void DumpItem(hadesmem::Process const& process, Item* item_ptr);
