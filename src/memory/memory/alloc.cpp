// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/alloc.hpp"

#include <utility>
#include <iostream>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

PVOID Alloc(Process const& process, SIZE_T size)
{
  PVOID const address = ::VirtualAllocEx(process.GetHandle(), nullptr, 
    size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!address)
  {
    DWORD const last_error = GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("VirtualAllocEx failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return address;
}

void Free(Process const& process, LPVOID address)
{
  if (!::VirtualFreeEx(process.GetHandle(), address, 0, MEM_RELEASE))
  {
    DWORD const last_error = GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("VirtualFreeEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
}

struct Allocator::Impl
{
  explicit Impl(Process const& process, SIZE_T size)
    : process_(&process), 
    base_(Alloc(process, size)), 
    size_(size)
  {
    BOOST_ASSERT(size != 0);
  }

  ~Impl()
  {
    FreeUnchecked();
  }

  void Free()
  {
    if (!process_)
    {
      return;
    }

    BOOST_ASSERT(base_ != nullptr);
    BOOST_ASSERT(size_ != 0);

    ::hadesmem::Free(*process_, base_);

    process_ = nullptr;
    base_ = nullptr;
    size_ = 0;
  }

  void FreeUnchecked() HADESMEM_NOEXCEPT
  {
    try
    {
      Free();
    }
    catch (std::exception const& e)
    {
      (void)e;

      // WARNING: Memory in remote process is leaked if 'Free' fails
      BOOST_ASSERT(boost::diagnostic_information(e).c_str() && false);

      process_ = nullptr;
      base_ = nullptr;
      size_ = 0;
    }
  }

  Process const* process_;
  PVOID base_;
  SIZE_T size_;
};

Allocator::Allocator(Process const& process, SIZE_T size)
  : impl_(new Impl(process, size))
{ }

Allocator::Allocator(Allocator&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

Allocator& Allocator::operator=(Allocator&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

Allocator::~Allocator()
{ }

void Allocator::Free()
{
  impl_->Free();
}

PVOID Allocator::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

SIZE_T Allocator::GetSize() const HADESMEM_NOEXCEPT
{
  return impl_->size_;
}

bool operator==(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(Allocator const& lhs, Allocator const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, Allocator const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, Allocator const& rhs)
{
  return (lhs << rhs.GetBase());
}

}
