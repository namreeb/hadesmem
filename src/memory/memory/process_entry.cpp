// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/process_entry.hpp"

namespace hadesmem
{

struct ProcessEntry::Impl
{
  Impl() 
    : id_(0), 
    threads_(0), 
    parent_(0), 
    priority_(0), 
    name_()
  { }

  Impl(PROCESSENTRY32 const& entry)
    : id_(entry.th32ProcessID), 
    threads_(entry.cntThreads), 
    parent_(entry.th32ParentProcessID), 
    priority_(entry.pcPriClassBase), 
    name_(entry.szExeFile)
  { }

  DWORD id_;
  DWORD threads_;
  DWORD parent_;
  LONG priority_;
  std::wstring name_;
};

ProcessEntry::ProcessEntry()
  : impl_(new Impl())
{ }

ProcessEntry::ProcessEntry(PROCESSENTRY32 const& entry)
  : impl_(new Impl(entry))
{ }

ProcessEntry::ProcessEntry(ProcessEntry const& other)
  : impl_(new Impl(*other.impl_))
{ }

ProcessEntry& ProcessEntry::operator=(ProcessEntry const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

ProcessEntry::ProcessEntry(ProcessEntry&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ProcessEntry& ProcessEntry::operator=(ProcessEntry&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

ProcessEntry::~ProcessEntry()
{ }

DWORD ProcessEntry::GetId() const HADESMEM_NOEXCEPT
{
  return impl_->id_;
}

DWORD ProcessEntry::GetThreads() const HADESMEM_NOEXCEPT
{
  return impl_->threads_;
}

DWORD ProcessEntry::GetParentId() const HADESMEM_NOEXCEPT
{
  return impl_->parent_;
}

LONG ProcessEntry::GetPriority() const HADESMEM_NOEXCEPT
{
  return impl_->priority_;
}

std::wstring ProcessEntry::GetName() const
{
  return impl_->name_;
}

}
