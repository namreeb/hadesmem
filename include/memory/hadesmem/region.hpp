// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <ostream>
#include <utility>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/query_region.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/protect.hpp>

namespace hadesmem
{

class Region
{
public:
  explicit Region(Process const& process, LPCVOID address)
    : process_(&process), mbi_(detail::Query(process, address))
  {
  }

  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return mbi_.BaseAddress;
  }

  PVOID GetAllocBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return mbi_.AllocationBase;
  }

  DWORD GetAllocProtect() const HADESMEM_DETAIL_NOEXCEPT
  {
    return mbi_.AllocationProtect;
  }

  SIZE_T GetSize() const HADESMEM_DETAIL_NOEXCEPT
  {
    return mbi_.RegionSize;
  }

  DWORD GetState() const HADESMEM_DETAIL_NOEXCEPT
  {
    return mbi_.State;
  }

  DWORD GetProtect() const HADESMEM_DETAIL_NOEXCEPT
  {
    return mbi_.Protect;
  }

  DWORD GetType() const HADESMEM_DETAIL_NOEXCEPT
  {
    return mbi_.Type;
  }

private:
  template <typename RegionT> friend class RegionIterator;

  explicit Region(Process const& process,
                  MEMORY_BASIC_INFORMATION const& mbi) HADESMEM_DETAIL_NOEXCEPT
    : process_(&process),
      mbi_(mbi)
  {
  }

  Process const* process_;
  MEMORY_BASIC_INFORMATION mbi_;
};

inline bool operator==(Region const& lhs, Region const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(Region const& lhs, Region const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Region const& lhs, Region const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(Region const& lhs, Region const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(Region const& lhs, Region const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(Region const& lhs, Region const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, Region const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Region const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}
}
