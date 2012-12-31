// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/process_list.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include "hadesmem/error.hpp"
#include "hadesmem/config.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/process_entry.hpp"
#include "hadesmem/detail/smart_handle.hpp"

namespace hadesmem
{

struct ProcessIterator::Impl
{
  explicit Impl() HADESMEM_NOEXCEPT
    : snap_(INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE), 
    process_()
  { }
  
  detail::SmartHandle snap_;
  boost::optional<ProcessEntry> process_;
};

ProcessIterator::ProcessIterator() HADESMEM_NOEXCEPT
  : impl_()
{ }

// TOOD: Clean this up.
ProcessIterator::ProcessIterator(int /*dummy*/)
  : impl_(new Impl())
{
  BOOST_ASSERT(impl_.get());
  
  impl_->snap_ = detail::SmartHandle(::CreateToolhelp32Snapshot(
    TH32CS_SNAPPROCESS, 0), INVALID_HANDLE_VALUE);
  if (!impl_->snap_.IsValid())
  {
    if (::GetLastError() == ERROR_BAD_LENGTH)
    {
      impl_->snap_ = detail::SmartHandle(::CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0), INVALID_HANDLE_VALUE);
      if (!impl_->snap_.IsValid())
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_THROW_EXCEPTION(Error() << 
          ErrorString("CreateToolhelp32Snapshot failed.") << 
          ErrorCodeWinLast(last_error));
      }
    }
    else
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("CreateToolhelp32Snapshot failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
  
  PROCESSENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!::Process32First(impl_->snap_.GetHandle(), &entry))
  {
    DWORD const last_error = ::GetLastError();
    
    if (last_error == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return;
    }
    
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Process32First failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  impl_->process_ = ProcessEntry(entry);
}

ProcessIterator::ProcessIterator(ProcessIterator const& other) 
  HADESMEM_NOEXCEPT
  : impl_(other.impl_)
{ }

ProcessIterator& ProcessIterator::operator=(ProcessIterator const& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = other.impl_;

  return *this;
}

ProcessIterator::ProcessIterator(ProcessIterator&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ProcessIterator& ProcessIterator::operator=(ProcessIterator&& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

ProcessIterator::~ProcessIterator()
{ }

ProcessIterator::reference ProcessIterator::operator*() const HADESMEM_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return *impl_->process_;
}

ProcessIterator::pointer ProcessIterator::operator->() const HADESMEM_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return &*impl_->process_;
}

ProcessIterator& ProcessIterator::operator++()
{
  BOOST_ASSERT(impl_.get());
  
  PROCESSENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!::Process32Next(impl_->snap_.GetHandle(), &entry))
  {
    if (::GetLastError() == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return *this;
    }
    else
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Module32Next failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
  
  impl_->process_ = ProcessEntry(entry);
  
  return *this;
}

ProcessIterator ProcessIterator::operator++(int)
{
  ProcessIterator iter(*this);
  ++*this;
  return iter;
}

bool ProcessIterator::operator==(ProcessIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool ProcessIterator::operator!=(ProcessIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return !(*this == other);
}

ProcessList::ProcessList() HADESMEM_NOEXCEPT
{ }

ProcessList::iterator ProcessList::begin()
{
  return ProcessList::iterator(0);
}

ProcessList::const_iterator ProcessList::begin() const
{
  return ProcessList::iterator(0);
}

ProcessList::iterator ProcessList::end() HADESMEM_NOEXCEPT
{
  return ProcessList::iterator();
}

ProcessList::const_iterator ProcessList::end() const HADESMEM_NOEXCEPT
{
  return ProcessList::iterator();
}

}
