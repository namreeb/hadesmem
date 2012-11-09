// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/detail/protect_guard.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/query_region.hpp"

namespace hadesmem
{

namespace detail
{

ProtectGuard::ProtectGuard(Process const* process, PVOID address, 
  ProtectGuardType type)
  : process_(process), 
  address_(address), 
  type_(type), 
  can_read_or_write_(false), 
  old_protect_(0)
{
  MEMORY_BASIC_INFORMATION const mbi = Query(*process_, address_);

  if (IsGuard(mbi))
  {
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("Attempt to write to guard page."));
  }

  can_read_or_write_ = (type_ == ProtectGuardType::kRead) ? 
    CanRead(mbi) : CanWrite(mbi);

  if (!can_read_or_write_)
  {
    old_protect_ = Protect(*process, address, PAGE_EXECUTE_READWRITE);
  }
}

ProtectGuard::ProtectGuard(ProtectGuard const& other) BOOST_NOEXCEPT
  : process_(other.process_), 
  address_(other.address_), 
  type_(other.type_), 
  can_read_or_write_(other.can_read_or_write_), 
  old_protect_(other.old_protect_)
{ }

ProtectGuard& ProtectGuard::operator=(ProtectGuard const& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  address_ = other.address_;
  type_ = other.type_;
  can_read_or_write_ = other.can_read_or_write_;
  old_protect_ = other.old_protect_;

  return *this;
}

ProtectGuard::ProtectGuard(ProtectGuard&& other) BOOST_NOEXCEPT
  : process_(other.process_), 
  address_(other.address_), 
  type_(other.type_), 
  can_read_or_write_(other.can_read_or_write_), 
  old_protect_(other.old_protect_)
{ }

ProtectGuard& ProtectGuard::operator=(ProtectGuard&& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  other.process_ = nullptr;

  address_ = other.address_;
  other.address_ = nullptr;

  type_ = other.type_;

  can_read_or_write_ = other.can_read_or_write_;
  other.can_read_or_write_ = false;

  old_protect_ = other.old_protect_;
  other.old_protect_ = 0;

  return *this;
}

ProtectGuard::~ProtectGuard()
{
  RestoreUnchecked();
}

void ProtectGuard::Restore()
{
  if (!old_protect_)
  {
    return;
  }

  if (!can_read_or_write_)
  {
    Protect(*process_, address_, old_protect_);
  }

  old_protect_ = 0;
}

void ProtectGuard::RestoreUnchecked() BOOST_NOEXCEPT
{
  try
  {
    Restore();
  }
  catch (std::exception const& e)
  {
    // WARNING: Protection is not restored if 'Restore' fails.

    (void)e;
    assert(boost::diagnostic_information(e).c_str() && false);
  }
}

}

}
