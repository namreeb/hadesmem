// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/module_list.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include "hadesmem/error.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/module_iterator_impl.hpp"

namespace hadesmem
{

ModuleIterator::ModuleIterator() BOOST_NOEXCEPT
  : impl_()
{ }

ModuleIterator::ModuleIterator(Process const* process)
  : impl_(new detail::ModuleIteratorImpl)
{
  BOOST_ASSERT(impl_.get());
  BOOST_ASSERT(process != nullptr);
  
  impl_->process_ = process;
  
  impl_->snap_ = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
    impl_->process_->GetId());
  if (impl_->snap_ == INVALID_HANDLE_VALUE)
  {
    if (::GetLastError() == ERROR_BAD_LENGTH)
    {
      impl_->snap_ = ::CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE, 
        impl_->process_->GetId());
      if (impl_->snap_ == INVALID_HANDLE_VALUE)
      {
        DWORD const last_error = ::GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorString("CreateToolhelp32Snapshot failed.") << 
          ErrorCodeWinLast(last_error));
      }
    }
    else
    {
      DWORD const last_error = ::GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("CreateToolhelp32Snapshot failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
  
  MODULEENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!::Module32First(impl_->snap_, &entry))
  {
    DWORD const last_error = ::GetLastError();
    if (last_error == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return;
    }
    
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("Module32First failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  impl_->module_ = Module(impl_->process_, entry);
}

ModuleIterator::ModuleIterator(ModuleIterator const& other) BOOST_NOEXCEPT
  : impl_(other.impl_)
{ }

ModuleIterator& ModuleIterator::operator=(ModuleIterator const& other) 
  BOOST_NOEXCEPT
{
  impl_ = other.impl_;
  
  return *this;
}

ModuleIterator::ModuleIterator(ModuleIterator&& other) BOOST_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ModuleIterator& ModuleIterator::operator=(ModuleIterator&& other) 
  BOOST_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

ModuleIterator::reference ModuleIterator::operator*() const BOOST_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return *impl_->module_;
}

ModuleIterator::pointer ModuleIterator::operator->() const BOOST_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return &*impl_->module_;
}

ModuleIterator& ModuleIterator::operator++()
{
  BOOST_ASSERT(impl_.get());
  
  MODULEENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!::Module32Next(impl_->snap_, &entry))
  {
    DWORD const last_error = ::GetLastError();
    if (last_error == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return *this;
    }
    
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("Module32Next failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  impl_->module_ = Module(impl_->process_, entry);
  
  return *this;
}

ModuleIterator ModuleIterator::operator++(int)
{
  ModuleIterator iter(*this);
  ++*this;
  return iter;
}

bool ModuleIterator::operator==(ModuleIterator const& other) const 
  BOOST_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool ModuleIterator::operator!=(ModuleIterator const& other) const 
  BOOST_NOEXCEPT
{
  return !(*this == other);
}

ModuleList::ModuleList(Process const* process) BOOST_NOEXCEPT
  : process_(process)
{
  BOOST_ASSERT(process != nullptr);
}

ModuleList::iterator ModuleList::begin()
{
  return ModuleList::iterator(process_);
}

ModuleList::const_iterator ModuleList::begin() const
{
  return ModuleList::iterator(process_);
}

ModuleList::iterator ModuleList::end() BOOST_NOEXCEPT
{
  return ModuleList::iterator();
}

ModuleList::const_iterator ModuleList::end() const BOOST_NOEXCEPT
{
  return ModuleList::iterator();
}

}
