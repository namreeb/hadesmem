// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <iterator>
#include <limits>

#include <windows.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/peb.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/region_list.hpp>

// TODO: Test hadesmem under large page mode. Especially getting module bases
// using VirtualQueryEx etc. Also see things like NtMapViewOfSection hook,
// IsSafeToUnload, PeFile, etc. Disassemble RtlPcToFileHeader to see what it
// does?

namespace hadesmem
{
namespace detail
{
// TODO: Rename this file.
inline SIZE_T GetModuleRegionSize(hadesmem::Process const& process,
                                  void const* base,
                                  bool break_on_bad_protect = true)
{
  hadesmem::Region r{process, base};

  hadesmem::RegionList regions{process};
  auto iter = std::find_if(
    std::begin(regions), std::end(regions), [&](Region const& region) {
      return region.GetAllocBase() == r.GetAllocBase();
    });
  SIZE_T size{};
  while (iter != std::end(regions) && iter->GetAllocBase() == r.GetAllocBase())
  {
    if (break_on_bad_protect &&
        hadesmem::IsBadProtect(process, iter->GetBase()))
    {
      break;
    }

    SIZE_T const region_size = iter->GetSize();
    size += region_size;
    HADESMEM_DETAIL_ASSERT(size >= region_size);
    ++iter;
  }

  HADESMEM_DETAIL_ASSERT(base >= r.GetAllocBase());
  size -= reinterpret_cast<std::intptr_t>(base) -
          reinterpret_cast<std::intptr_t>(r.GetAllocBase());

  if (!size)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"Invalid region allocation size."});
  }

  return size;
}
}
}
