// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/detail/protect_guard.hpp"

#include <utility>

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/query_region.hpp"
#include "hadesmem/detail/protect_region.hpp"

namespace hadesmem
{

namespace detail
{

struct ProtectGuard::Impl
{
  explicit Impl(Process const& process, PVOID address, ProtectGuardType type)
    : process_(&process), 
    type_(type), 
    can_read_or_write_(false), 
    old_protect_(0), 
    mbi_(Query(process, address))
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
      old_protect_ = Protect(process, mbi_, PAGE_EXECUTE_READWRITE);
    }
  }

  ~Impl()
  {
    RestoreUnchecked();
  }

  void Restore()
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

  void RestoreUnchecked() HADESMEM_NOEXCEPT
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

  Process const* process_;
  ProtectGuardType type_;
  bool can_read_or_write_;
  DWORD old_protect_;
  MEMORY_BASIC_INFORMATION mbi_;
};

ProtectGuard::ProtectGuard(Process const& process, PVOID address, 
  ProtectGuardType type)
  : impl_(new Impl(process, address, type))
{ }

ProtectGuard::ProtectGuard(ProtectGuard&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ProtectGuard& ProtectGuard::operator=(ProtectGuard&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

ProtectGuard::~ProtectGuard()
{ }

void ProtectGuard::Restore()
{
  impl_->Restore();
}

void ProtectGuard::RestoreUnchecked() HADESMEM_NOEXCEPT
{
  impl_->RestoreUnchecked();
}

}

}
