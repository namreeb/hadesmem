// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <iterator>
#include <limits>

#include <windows.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/region_list.hpp>

namespace hadesmem
{

namespace detail
{

inline SIZE_T GetRegionAllocSize(hadesmem::Process const& process,
                                 void const* base)
{
  hadesmem::RegionList regions{process};
  auto iter = std::find_if(std::begin(regions),
                           std::end(regions),
                           [&](Region const& region)
                           { return region.GetAllocBase() == base; });
  SIZE_T size{};
  while (iter != std::end(regions) && iter->GetAllocBase() == base)
  {
    SIZE_T const region_size = iter->GetSize();
    HADESMEM_DETAIL_ASSERT(region_size < (std::numeric_limits<DWORD>::max)());
    size += static_cast<DWORD>(region_size);
    HADESMEM_DETAIL_ASSERT(size >= region_size);
    ++iter;
  }
  return size;
}
}
}
