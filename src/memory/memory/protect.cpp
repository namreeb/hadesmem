// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/protect.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/query_region.hpp"

namespace hadesmem
{

bool CanRead(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::CanRead(mbi);
}

bool CanWrite(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::CanWrite(mbi);
}

bool CanExecute(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::CanExecute(mbi);
}

bool IsGuard(Process const& process, LPCVOID address)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  return detail::IsGuard(mbi);
}

DWORD Protect(Process const& process, LPVOID address, DWORD protect)
{
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, address);
  
  DWORD old_protect = 0;
  if (!::VirtualProtectEx(process.GetHandle(), mbi.BaseAddress, mbi.RegionSize, 
    protect, &old_protect))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("VirtualProtectEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return old_protect;
}

}
