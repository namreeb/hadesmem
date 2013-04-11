// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/module_list.hpp>

#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/optional.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/smart_handle.hpp>

namespace hadesmem
{

struct ModuleIterator::Impl
{
  explicit Impl() HADESMEM_NOEXCEPT
    : process_(nullptr), 
    snap_(INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE), 
    module_()
  { }
  
  Process const* process_;
  detail::SmartHandle snap_;
  boost::optional<Module> module_;
};

ModuleIterator::ModuleIterator() HADESMEM_NOEXCEPT
  : impl_()
{ }

// TOOD: Clean this up.
ModuleIterator::ModuleIterator(Process const& process)
  : impl_(new Impl())
{
  HADESMEM_ASSERT(impl_.get());
  
  impl_->process_ = &process;
  
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
  
  impl_->module_ = Module(*impl_->process_, entry);
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

ModuleIterator::~ModuleIterator()
{ }

ModuleIterator::reference ModuleIterator::operator*() const HADESMEM_NOEXCEPT
{
  HADESMEM_ASSERT(impl_.get());
  return *impl_->module_;
}

ModuleIterator::pointer ModuleIterator::operator->() const HADESMEM_NOEXCEPT
{
  HADESMEM_ASSERT(impl_.get());
  return &*impl_->module_;
}

ModuleIterator& ModuleIterator::operator++()
{
  HADESMEM_ASSERT(impl_.get());
  
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
  
  impl_->module_ = Module(*impl_->process_, entry);
  
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

struct ModuleList::Impl
{
  Impl(Process const& process) HADESMEM_NOEXCEPT
    : process_(&process)
  { }

  Process const* process_;
};

ModuleList::ModuleList(Process const& process)
  : impl_(new Impl(process))
{ }

ModuleList::ModuleList(ModuleList const& other)
  : impl_(new Impl(*other.impl_))
{ }

ModuleList& ModuleList::operator=(ModuleList const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

ModuleList::ModuleList(ModuleList&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ModuleList& ModuleList::operator=(ModuleList&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

ModuleList::~ModuleList()
{ }

ModuleList::iterator ModuleList::begin()
{
  return ModuleList::iterator(*impl_->process_);
}

ModuleList::const_iterator ModuleList::begin() const
{
  return ModuleList::iterator(*impl_->process_);
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
