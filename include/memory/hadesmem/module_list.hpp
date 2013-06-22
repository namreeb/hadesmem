// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <utility>
#include <iterator>

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

// ModuleIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ModuleIterator : public std::iterator<std::input_iterator_tag, Module>
{
public:
  ModuleIterator() HADESMEM_NOEXCEPT
    : impl_()
  { }
  
// TOOD: Clean this up.
  explicit ModuleIterator(Process const& process)
    : impl_(new Impl())
  {
    HADESMEM_ASSERT(impl_.get());
  
    impl_->process_ = &process;
  
    impl_->snap_ = detail::SmartSnapHandle(
      ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
      impl_->process_->GetId()));
    if (!impl_->snap_.IsValid())
    {
      if (::GetLastError() == ERROR_BAD_LENGTH)
      {
        impl_->snap_ = detail::SmartSnapHandle(
          ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
          impl_->process_->GetId()));
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
    entry.dwSize = static_cast<DWORD>(sizeof(entry));
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

  ModuleIterator(ModuleIterator const& other) HADESMEM_NOEXCEPT
    : impl_(other.impl_)
  { }

  ModuleIterator& operator=(ModuleIterator const& other) HADESMEM_NOEXCEPT
  {
    impl_ = other.impl_;

    return *this;
  }

  ModuleIterator(ModuleIterator&& other) HADESMEM_NOEXCEPT
    : impl_(std::move(other.impl_))
  { }

  ModuleIterator& operator=(ModuleIterator&& other) HADESMEM_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

  ~ModuleIterator() HADESMEM_NOEXCEPT
  { }
  
  reference operator*() const HADESMEM_NOEXCEPT
  {
    HADESMEM_ASSERT(impl_.get());
    return *impl_->module_;
  }

  pointer operator->() const HADESMEM_NOEXCEPT
  {
    HADESMEM_ASSERT(impl_.get());
    return &*impl_->module_;
  }

  ModuleIterator& operator++()
  {
    HADESMEM_ASSERT(impl_.get());
  
    MODULEENTRY32 entry;
    ::ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = static_cast<DWORD>(sizeof(entry));
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
  
  ModuleIterator operator++(int)
  {
    ModuleIterator iter(*this);
    ++*this;
    return iter;
  }
  
  bool operator==(ModuleIterator const& other) const HADESMEM_NOEXCEPT
  {
    return impl_ == other.impl_;
  }
  
  bool operator!=(ModuleIterator const& other) const HADESMEM_NOEXCEPT
  {
    return impl_ != other.impl_;
  }
  
private:
  struct Impl
  {
    Impl() HADESMEM_NOEXCEPT
      : process_(nullptr), 
      snap_(INVALID_HANDLE_VALUE), 
      module_()
    { }

    Process const* process_;
    detail::SmartSnapHandle snap_;
    boost::optional<Module> module_;
  };

  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class ModuleList
{
public:
  typedef Module value_type;
  typedef ModuleIterator iterator;
  typedef ModuleIterator const_iterator;
  
  explicit HADESMEM_CONSTEXPR ModuleList(Process const& process) 
    HADESMEM_NOEXCEPT
    : process_(&process)
  { }

  HADESMEM_CONSTEXPR ModuleList(ModuleList const& other) HADESMEM_NOEXCEPT
    : process_(other.process_)
  { }

  ModuleList& operator=(ModuleList const& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;

    return *this;
  }

  HADESMEM_CONSTEXPR ModuleList(ModuleList&& other) HADESMEM_NOEXCEPT
    : process_(other.process_)
  { }

  ModuleList& operator=(ModuleList&& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;

    return *this;
  }

  ~ModuleList() HADESMEM_NOEXCEPT
  { }
  
  iterator begin()
  {
    return iterator(*process_);
  }
  
  const_iterator begin() const
  {
    return const_iterator(*process_);
  }
  
  iterator end() HADESMEM_NOEXCEPT
  {
    return iterator();
  }
  
  const_iterator end() const HADESMEM_NOEXCEPT
  {
    return const_iterator();
  }
  
private:
  Process const* process_;
};

}
