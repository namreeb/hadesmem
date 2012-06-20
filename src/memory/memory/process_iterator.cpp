// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/process_iterator.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/process_iterator_impl.hpp"

namespace hadesmem
{

ProcessIterator::ProcessIterator() BOOST_NOEXCEPT
  : impl_()
{ }

ProcessIterator::ProcessIterator(int /*dummy*/)
  : impl_(new detail::ProcessIteratorImpl)
{
  impl_->snap_ = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (impl_->snap_ == INVALID_HANDLE_VALUE)
  {
    if (GetLastError() == ERROR_BAD_LENGTH)
    {
      impl_->snap_ = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
      if (impl_->snap_ == INVALID_HANDLE_VALUE)
      {
        DWORD const last_error = GetLastError();
        BOOST_THROW_EXCEPTION(HadesMemError() << 
          ErrorString("CreateToolhelp32Snapshot failed.") << 
          ErrorCodeWinLast(last_error));
      }
    }
    else
    {
      DWORD const last_error = GetLastError();
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("CreateToolhelp32Snapshot failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
  
  PROCESSENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!::Process32First(impl_->snap_, &entry))
  {
    DWORD const last_error = ::GetLastError();
    
    if (last_error == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return;
    }
    
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Process32First failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  ProcessEntry new_entry;
  new_entry.id = entry.th32ProcessID;
  new_entry.threads = entry.cntThreads;
  new_entry.priority = entry.pcPriClassBase;
  new_entry.name = entry.szExeFile;
  
  impl_->process_ = new_entry;
}

ProcessIterator::ProcessIterator(ProcessIterator const& other) BOOST_NOEXCEPT
  : impl_(other.impl_)
{ }

ProcessIterator& ProcessIterator::operator=(ProcessIterator const& other) 
  BOOST_NOEXCEPT
{
  impl_ = other.impl_;
  
  return *this;
}

ProcessIterator::ProcessIterator(ProcessIterator&& other) BOOST_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ProcessIterator& ProcessIterator::operator=(ProcessIterator&& other) 
  BOOST_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

ProcessIterator::~ProcessIterator() BOOST_NOEXCEPT
{ }

ProcessIterator::reference ProcessIterator::operator*() const BOOST_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return *impl_->process_;
}

ProcessIterator::pointer ProcessIterator::operator->() const BOOST_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return &*impl_->process_;
}

ProcessIterator& ProcessIterator::operator++()
{
  PROCESSENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!::Process32Next(impl_->snap_, &entry))
  {
    if (::GetLastError() == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return *this;
    }
    else
    {
      DWORD const last_error = ::GetLastError();
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("Module32Next failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
  
  ProcessEntry new_entry;
  new_entry.id = entry.th32ProcessID;
  new_entry.threads = entry.cntThreads;
  new_entry.priority = entry.pcPriClassBase;
  new_entry.name = entry.szExeFile;
  
  impl_->process_ = new_entry;
  
  return *this;
}

ProcessIterator ProcessIterator::operator++(int)
{
  ProcessIterator iter(*this);
  ++*this;
  return iter;
}

bool ProcessIterator::operator==(ProcessIterator const& other) BOOST_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool ProcessIterator::operator!=(ProcessIterator const& other) BOOST_NOEXCEPT
{
  return !(*this == other);
}

}
