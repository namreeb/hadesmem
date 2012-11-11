// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/module_list.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include "hadesmem/error.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/module_iterator_impl.hpp"

namespace hadesmem
{

ModuleIterator::ModuleIterator() HADESMEM_NOEXCEPT
  : impl_()
{ }

ModuleIterator::ModuleIterator(Process const* process)
  : impl_(new detail::ModuleIteratorImpl)
{
  assert(impl_.get());
  assert(process != nullptr);
  
  impl_->process_ = process;
  
  impl_->snap_ = detail::SmartHandle(
    ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
    impl_->process_->GetId()), INVALID_HANDLE_VALUE);
  if (!impl_->snap_.IsValid())
  {
    if (::GetLastError() == ERROR_BAD_LENGTH)
    {
      impl_->snap_ = detail::SmartHandle(
        ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
        impl_->process_->GetId()), INVALID_HANDLE_VALUE);
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
  
  MODULEENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!::Module32First(impl_->snap_.GetHandle(), &entry))
  {
    DWORD const last_error = ::GetLastError();
    if (last_error == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return;
    }
    
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Module32First failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  impl_->module_ = Module(impl_->process_, entry);
}

ModuleIterator::ModuleIterator(ModuleIterator const& other) HADESMEM_NOEXCEPT
  : impl_(other.impl_)
{ }

ModuleIterator& ModuleIterator::operator=(ModuleIterator const& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = other.impl_;
  
  return *this;
}

ModuleIterator::ModuleIterator(ModuleIterator&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ModuleIterator& ModuleIterator::operator=(ModuleIterator&& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

ModuleIterator::reference ModuleIterator::operator*() const HADESMEM_NOEXCEPT
{
  assert(impl_.get());
  return *impl_->module_;
}

ModuleIterator::pointer ModuleIterator::operator->() const HADESMEM_NOEXCEPT
{
  assert(impl_.get());
  return &*impl_->module_;
}

ModuleIterator& ModuleIterator::operator++()
{
  assert(impl_.get());
  
  MODULEENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!::Module32Next(impl_->snap_.GetHandle(), &entry))
  {
    DWORD const last_error = ::GetLastError();
    if (last_error == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return *this;
    }
    
    HADESMEM_THROW_EXCEPTION(Error() << 
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
  HADESMEM_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool ModuleIterator::operator!=(ModuleIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return !(*this == other);
}

ModuleList::ModuleList(Process const* process) HADESMEM_NOEXCEPT
  : process_(process)
{
  assert(process != nullptr);
}

ModuleList::iterator ModuleList::begin()
{
  return ModuleList::iterator(process_);
}

ModuleList::const_iterator ModuleList::begin() const
{
  return ModuleList::iterator(process_);
}

ModuleList::iterator ModuleList::end() HADESMEM_NOEXCEPT
{
  return ModuleList::iterator();
}

ModuleList::const_iterator ModuleList::end() const HADESMEM_NOEXCEPT
{
  return ModuleList::iterator();
}

}
