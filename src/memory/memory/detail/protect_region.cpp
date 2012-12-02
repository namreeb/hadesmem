// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/detail/protect_region.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

namespace detail
{

DWORD Protect(Process const& process, MEMORY_BASIC_INFORMATION const& mbi, 
  DWORD protect)
{
  DWORD old_protect = 0;
  if (!::VirtualProtectEx(process.GetHandle(), mbi.BaseAddress, mbi.RegionSize, 
    protect, &old_protect))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("VirtualProtectEx failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return old_protect;
}

}

}
