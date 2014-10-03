// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>

#include <hadesmem/config.hpp>

#include "flagged.hpp"
#include "static_assert.hpp"
#include "std_string.hpp"

namespace divinity
{

// Need to figure out how to replicate the vtable layout from the game.
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif // #if defined(HADESMEM_GCC)

struct ItemTemplate
{
  virtual bool Serialize(unsigned int a2) = 0;

  virtual char** GetTypeName() = 0;

  virtual void Unknown0008() = 0;

  virtual void Unknown000C() = 0;

  virtual void Unknown0010() = 0;

  virtual void Unknown0014() = 0;

  virtual void Destructor() = 0;

  virtual void Unknown001C() = 0;

  virtual void Unknown0020() = 0;

  virtual void Unknown0024() = 0;

  virtual void Unknown0028() = 0;

  virtual void Unknown002C() = 0;

  virtual void Unknown0030() = 0;

  virtual void Unknown0034() = 0;

  virtual void Unknown0038() = 0;

  virtual void Unknown003C() = 0;

  int field_4;
  int field_8;
  char* uuid_;
  StdStringA name_;
  int field_2C;
  int field_30;
  int field_34[114];
  FlaggedCString icon_;
  FlaggedBool can_be_picked_up_;
  FlaggedBool can_be_moved_;
  FlaggedBool can_see_through_;
  FlaggedBool can_shoot_through_;
  FlaggedBool can_click_through_;
  FlaggedBool destroyed_;
  FlaggedBool walk_through_;
  FlaggedBool walk_on_;
  FlaggedBool wadable_;
  FlaggedBool story_item_;
  FlaggedBool is_key_;
  FlaggedBool is_trap_;
  FlaggedBool is_surface_interactable_;
  FlaggedBool is_surface_cloud_interactable_;
  FlaggedBool treasure_on_destroy_;
  std::int16_t field_222;
  int unknown_tooltip_related_;
  int field_228;
  FlaggedCString stats_;
  int field_234;
  int field_238;
  int field_23C;
  int field_240;
  int field_244;
  int on_use_peace_actions_;
  int field_24C;
  int field_250;
  int field_254;
  int field_258;
  int on_destroy_actions_;
  int field_260;
  int field_264;
  int field_268;
  int field_26C;
  int unknown_scripts_related_;
  int field_274;
  int field_278;
  int field_27C;
  int field_280;
  FlaggedCString default_state_;
  FlaggedCString owner_;
  FlaggedCString key_;
  FlaggedCString hit_fx_;
  FlaggedDword lock_level_;
  FlaggedDword amount_;
  FlaggedDword max_stack_amount_;
  int field_2BC;
  int field_2C0;
  FlaggedCString drop_sound_;
  FlaggedCString pickup_sound_;
  FlaggedCString use_sound_;
  FlaggedCString loop_sound_;
  FlaggedCString sound_switch_;
  int blood_surface_type_;
  int field_2F0;
  int field_2F4;
  int field_2F8;
  FlaggedTriStringPairPoly description_;
  FlaggedCString speaker_;
  FlaggedCString alt_speaker_;
  FlaggedCString spaker_greeting_override_;
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

HADESMEM_DETAIL_STATIC_ASSERT_X86(sizeof(ItemTemplate) == 0x39C);
}
