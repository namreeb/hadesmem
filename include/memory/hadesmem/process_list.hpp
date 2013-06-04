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
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/detail/smart_handle.hpp>

namespace hadesmem
{

// ProcessIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ProcessIterator : public std::iterator<std::input_iterator_tag, 
  ProcessEntry>
{
public:
  ProcessIterator() HADESMEM_NOEXCEPT
    : impl_()
  { }
  
  // TOOD: Clean this up.
  ProcessIterator(int /*dummy*/)
    : impl_(new Impl())
  {
    HADESMEM_ASSERT(impl_.get());
  
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
    entry.dwSize = static_cast<DWORD>(sizeof(entry));
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

  ProcessIterator(ProcessIterator const& other) HADESMEM_NOEXCEPT
    : impl_(other.impl_)
  { }

  ProcessIterator& operator=(ProcessIterator const& other) HADESMEM_NOEXCEPT
  {
    impl_ = other.impl_;

    return *this;
  }

  ProcessIterator(ProcessIterator&& other) HADESMEM_NOEXCEPT
    : impl_(std::move(other.impl_))
  { }

  ProcessIterator& operator=(ProcessIterator&& other) HADESMEM_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

  ~ProcessIterator() HADESMEM_NOEXCEPT
  { }
  
  reference operator*() const HADESMEM_NOEXCEPT
  {
    HADESMEM_ASSERT(impl_.get());
    return *impl_->process_;
  }
  
  pointer operator->() const HADESMEM_NOEXCEPT
  {
    HADESMEM_ASSERT(impl_.get());
    return &*impl_->process_;
  }
  
  ProcessIterator& operator++()
  {
    HADESMEM_ASSERT(impl_.get());
  
    PROCESSENTRY32 entry;
    ::ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = static_cast<DWORD>(sizeof(entry));
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
  
  ProcessIterator operator++(int)
  {
    ProcessIterator iter(*this);
    ++*this;
    return iter;
  }
  
  bool operator==(ProcessIterator const& other) const HADESMEM_NOEXCEPT
  {
    return impl_ == other.impl_;
  }
  
  bool operator!=(ProcessIterator const& other) const HADESMEM_NOEXCEPT
  {
    return !(*this == other);
  }
  
private:
  struct Impl
  {
    explicit Impl() HADESMEM_NOEXCEPT
      : snap_(INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE), 
      process_()
    { }
  
    detail::SmartHandle snap_;
    boost::optional<ProcessEntry> process_;
  };

  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class ProcessList
{
public:
  typedef ProcessIterator iterator;
  typedef ProcessIterator const_iterator;

  HADESMEM_CONSTEXPR ProcessList() HADESMEM_NOEXCEPT
  { }

  iterator begin()
  {
    return iterator(0);
  }

  const_iterator begin() const
  {
    return const_iterator(0);
  }

  iterator end() HADESMEM_NOEXCEPT
  {
    return iterator();
  }

  const_iterator end() const HADESMEM_NOEXCEPT
  {
    return const_iterator();
  }
};

}
