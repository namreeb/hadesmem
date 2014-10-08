// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "item_manager.hpp"

#include <hadesmem/detail/trace.hpp>

void DumpItemManager(divinity::ItemManager* item_manager)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Got item manager: %p.", item_manager);
  for (int i = 0; i < item_manager->item_ptr_array_len_; ++i)
  {
    if (auto item = item_manager->item_ptr_array_[i])
    {
      DumpItem(item, i);
    }
  }
}
