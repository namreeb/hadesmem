// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/pefile.hpp"

#include <utility>
#include <iostream>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

struct PeFile::Impl
{
  explicit Impl(Process const& process, PVOID address, PeFileType type)
    : process_(&process), 
    base_(address), 
    type_(type)
  {
    BOOST_ASSERT(base_ != 0);
  }

  Process const* process_;
  PVOID base_;
  PeFileType type_;
};

PeFile::PeFile(Process const& process, PVOID address, PeFileType type)
  : impl_(new Impl(process, address, type))
{ }

PeFile::PeFile(PeFile const& other)
  : impl_(new Impl(*other.impl_))
{ }

PeFile& PeFile::operator=(PeFile const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

PeFile::PeFile(PeFile&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

PeFile& PeFile::operator=(PeFile&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

PeFile::~PeFile()
{ }

PVOID PeFile::GetBase() const HADESMEM_NOEXCEPT
{
  return impl_->base_;
}

bool operator==(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

bool operator!=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

bool operator<=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

bool operator>(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

bool operator>=(PeFile const& lhs, PeFile const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

std::ostream& operator<<(std::ostream& lhs, PeFile const& rhs)
{
  return (lhs << rhs.GetBase());
}

std::wostream& operator<<(std::wostream& lhs, PeFile const& rhs)
{
  return (lhs << rhs.GetBase());
}

}
