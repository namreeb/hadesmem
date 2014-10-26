// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/config.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/process.hpp>

struct Inventory
{
  virtual ~Inventory()
  {
  }

  virtual void Unknown0004()
  {
  }

  int field_0[7];
  int field_20;
  unsigned int parent_;
  int field_28;
  int gold_;
  int field_30;
  int field_34;
  unsigned int* item_handles_;
  int field_3C;
  int capacity_;
  int field_44;
  // int field_48[100];
};

#if defined(HADESMEM_DETAIL_ARCH_X86)
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(Inventory) == 0x48);
#endif // #if defined(HADESMEM_DETAIL_ARCH_X86)

void DumpInventory(hadesmem::Process const& process, Inventory* inventory_ptr);
