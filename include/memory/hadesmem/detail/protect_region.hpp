// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{
namespace detail
{
inline DWORD Protect(Process const& process,
                     MEMORY_BASIC_INFORMATION const& mbi,
                     DWORD protect)
{
  DWORD old_protect = 0;
  if (!::VirtualProtectEx(process.GetHandle(),
                          mbi.BaseAddress,
                          mbi.RegionSize,
                          protect,
                          &old_protect))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{}
                                    << ErrorString{"VirtualProtectEx failed."}
                                    << ErrorCodeWinLast{last_error});
  }

  return old_protect;
}
}
}
