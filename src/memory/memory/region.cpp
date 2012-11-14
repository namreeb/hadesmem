// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/region.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/query_region.hpp"

namespace hadesmem
{

Region::Region(Process const* process, LPCVOID address)
  : process_(process), 
  mbi_(detail::Query(*process, address))
{
  assert(process != nullptr);
}

Region::Region(Process const* process, MEMORY_BASIC_INFORMATION const& mbi) 
  HADESMEM_NOEXCEPT
  : process_(process), 
  mbi_(mbi)
{
  assert(process != nullptr);
}

PVOID Region::GetBase() const HADESMEM_NOEXCEPT
{
  return mbi_.BaseAddress;
}

PVOID Region::GetAllocBase() const HADESMEM_NOEXCEPT
{
  return mbi_.AllocationBase;
}

DWORD Region::GetAllocProtect() const HADESMEM_NOEXCEPT
{
  return mbi_.AllocationProtect;
}

SIZE_T Region::GetSize() const HADESMEM_NOEXCEPT
{
  return mbi_.RegionSize;
}

DWORD Region::GetState() const HADESMEM_NOEXCEPT
{
  return mbi_.State;
}

DWORD Region::GetProtect() const HADESMEM_NOEXCEPT
{
  return mbi_.Protect;
}

DWORD Region::GetType() const HADESMEM_NOEXCEPT
{
  return mbi_.Type;
}

bool operator==(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(Region const& lhs, Region const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, Region const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, Region const& rhs)
{
  return (lhs << rhs.GetBase());
}

}
