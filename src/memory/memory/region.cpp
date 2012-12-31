// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/region.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <ostream>
#include <utility>

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/query_region.hpp"

namespace hadesmem
{

struct Region::Impl
{
  explicit Impl(Process const* process, LPCVOID address)
    : process_(process), 
    mbi_(detail::Query(*process, address))
  {
    BOOST_ASSERT(process != nullptr);
  }

  explicit Impl(Process const* process, MEMORY_BASIC_INFORMATION const& mbi) 
    HADESMEM_NOEXCEPT
    : process_(process), 
    mbi_(mbi)
  {
    BOOST_ASSERT(process != nullptr);
  }

  Process const* process_;
  MEMORY_BASIC_INFORMATION mbi_;
};

Region::Region(Process const* process, LPCVOID address)
  : impl_(new Impl(process, address))
{ }

Region::Region(Process const* process, MEMORY_BASIC_INFORMATION const& mbi)
  : impl_(new Impl(process, mbi))
{ }

Region::Region(Region const& other)
  : impl_(new Impl(*other.impl_))
{ }

Region& Region::operator=(Region const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

Region::Region(Region&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

Region& Region::operator=(Region&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

Region::~Region()
{ }

PVOID Region::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->mbi_.BaseAddress;
}

PVOID Region::GetAllocBase() const HADESMEM_NOEXCEPT
{
  return impl_->mbi_.AllocationBase;
}

DWORD Region::GetAllocProtect() const HADESMEM_NOEXCEPT
{
  return impl_->mbi_.AllocationProtect;
}

SIZE_T Region::GetSize() const HADESMEM_NOEXCEPT
{
  return impl_->mbi_.RegionSize;
}

DWORD Region::GetState() const HADESMEM_NOEXCEPT
{
  return impl_->mbi_.State;
}

DWORD Region::GetProtect() const HADESMEM_NOEXCEPT
{
  return impl_->mbi_.Protect;
}

DWORD Region::GetType() const HADESMEM_NOEXCEPT
{
  return impl_->mbi_.Type;
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
