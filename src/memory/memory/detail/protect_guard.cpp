// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/detail/protect_guard.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/query_region.hpp"
#include "hadesmem/detail/protect_region.hpp"

namespace hadesmem
{

namespace detail
{

ProtectGuard::ProtectGuard(Process const* process, PVOID address, 
  ProtectGuardType type)
  : process_(process), 
  type_(type), 
  can_read_or_write_(false), 
  old_protect_(0), 
  mbi_(Query(*process_, address))
{
  if (IsGuard(mbi_))
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Attempt to write to guard page."));
  }

  can_read_or_write_ = (type_ == ProtectGuardType::kRead) ? 
    CanRead(mbi_) : CanWrite(mbi_);

  if (!can_read_or_write_)
  {
    old_protect_ = Protect(*process, mbi_, PAGE_EXECUTE_READWRITE);
  }
}

ProtectGuard::ProtectGuard(ProtectGuard&& other) HADESMEM_NOEXCEPT
  : process_(other.process_), 
  type_(other.type_), 
  can_read_or_write_(other.can_read_or_write_), 
  old_protect_(other.old_protect_), 
  mbi_(other.mbi_)
{ }

ProtectGuard& ProtectGuard::operator=(ProtectGuard&& other) HADESMEM_NOEXCEPT
{
  process_ = other.process_;
  other.process_ = nullptr;

  type_ = other.type_;

  can_read_or_write_ = other.can_read_or_write_;
  other.can_read_or_write_ = false;

  old_protect_ = other.old_protect_;
  other.old_protect_ = 0;

  mbi_ = other.mbi_;

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
    Protect(*process_, mbi_, old_protect_);
  }

  old_protect_ = 0;
}

void ProtectGuard::RestoreUnchecked() HADESMEM_NOEXCEPT
{
  try
  {
    Restore();
  }
  catch (std::exception const& e)
  {
    (void)e;

    // WARNING: Protection is not restored if 'Restore' fails.
    assert(boost::diagnostic_information(e).c_str() && false);
  }
}

}

}
