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
#include <hadesmem/thread_entry.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/smart_handle.hpp>

// TODO: Add tests for ThreadList and ThreadIterator.

namespace hadesmem
{

// ThreadIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename ThreadEntryT>
class ThreadIterator : public std::iterator<std::input_iterator_tag, 
  ThreadEntryT>
{
public:
  typedef std::iterator<std::input_iterator_tag, ThreadEntryT> BaseIteratorT;
  typedef typename BaseIteratorT::value_type value_type;
  typedef typename BaseIteratorT::difference_type difference_type;
  typedef typename BaseIteratorT::pointer pointer;
  typedef typename BaseIteratorT::reference reference;
  typedef typename BaseIteratorT::iterator_category iterator_category;

  ThreadIterator() HADESMEM_NOEXCEPT
    : impl_(), 
    pid_(0)
  { }

  // TODO: Clean this up.
  ThreadIterator(DWORD pid) HADESMEM_NOEXCEPT
    : impl_(std::make_shared<Impl>()), 
    pid_(pid)
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    
    impl_->snap_ = detail::SmartSnapHandle(::CreateToolhelp32Snapshot(
      TH32CS_SNAPTHREAD, 0));
    if (!impl_->snap_.IsValid())
    {
      if (::GetLastError() == ERROR_BAD_LENGTH)
      {
        impl_->snap_ = detail::SmartSnapHandle(::CreateToolhelp32Snapshot(
          TH32CS_SNAPTHREAD, 0));
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
    
    THREADENTRY32 entry;
    ::ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = static_cast<DWORD>(sizeof(entry));
    if (!::Thread32First(impl_->snap_.GetHandle(), &entry))
    {
      DWORD const last_error = ::GetLastError();
    
      if (last_error == ERROR_NO_MORE_FILES)
      {
        impl_.reset();
        return;
      }
    
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Thread32First failed.") << 
        ErrorCodeWinLast(last_error));
    }
    
    if (IsTargetThread(entry.th32OwnerProcessID))
    {
      impl_->thread_ = ThreadEntry(entry);
    }
    else
    {
      Advance();
    }
  }

  ThreadIterator(ThreadIterator const& other) HADESMEM_NOEXCEPT
    : impl_(other.impl_), 
    pid_(other.pid_)
  { }

  ThreadIterator& operator=(ThreadIterator const& other) HADESMEM_NOEXCEPT
  {
    impl_ = other.impl_;
    pid_ = other.pid_;

    return *this;
  }

  ThreadIterator(ThreadIterator&& other) HADESMEM_NOEXCEPT
    : impl_(std::move(other.impl_)), 
    pid_(other.pid_)
  { }

  ThreadIterator& operator=(ThreadIterator&& other) HADESMEM_NOEXCEPT
  {
    impl_ = std::move(other.impl_);
    pid_ = other.pid_;

    return *this;
  }

  ~ThreadIterator() HADESMEM_NOEXCEPT
  { }
  
  reference operator*() const HADESMEM_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->thread_;
  }
  
  pointer operator->() const HADESMEM_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->thread_;
  }
  
  ThreadIterator& operator++()
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    
    Advance();
    
    return *this;
  }
  
  ThreadIterator operator++(int)
  {
    ThreadIterator iter(*this);
    ++*this;
    return iter;
  }
  
  bool operator==(ThreadIterator const& other) const HADESMEM_NOEXCEPT
  {
    return impl_ == other.impl_;
  }
  
  bool operator!=(ThreadIterator const& other) const HADESMEM_NOEXCEPT
  {
    return !(*this == other);
  }
  
private:
  void Advance()
  {
    THREADENTRY32 entry;
    ::ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = static_cast<DWORD>(sizeof(entry));
    for (;;) 
    {
      if (!::Thread32Next(impl_->snap_.GetHandle(), &entry))
      {
        if (::GetLastError() == ERROR_NO_MORE_FILES)
        {
          impl_.reset();
          break;
        }
        else
        {
          DWORD const last_error = ::GetLastError();
          HADESMEM_THROW_EXCEPTION(Error() << 
            ErrorString("Thread32Next failed.") << 
            ErrorCodeWinLast(last_error));
        }
      }

      if (IsTargetThread(entry.th32OwnerProcessID))
      {
        impl_->thread_ = ThreadEntry(entry);
        break;
      }
    }
  }

  bool IsTargetThread(DWORD owner_id)
  {
    return pid_ == static_cast<DWORD>(-1) || pid_ == owner_id;
  }
  
  struct Impl
  {
    Impl() HADESMEM_NOEXCEPT
      : snap_(INVALID_HANDLE_VALUE), 
      thread_()
    { }
  
    detail::SmartSnapHandle snap_;
    boost::optional<ThreadEntry> thread_;
  };

  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<Impl> impl_;
  DWORD pid_;
};

class ThreadList
{
public:
  typedef ThreadIterator<ThreadEntry> iterator;
  typedef ThreadIterator<ThreadEntry const> const_iterator;

  HADESMEM_CONSTEXPR ThreadList() HADESMEM_NOEXCEPT
    : pid_(static_cast<DWORD>(-1))
  { }

  HADESMEM_CONSTEXPR ThreadList(DWORD pid) HADESMEM_NOEXCEPT
    : pid_(pid)
  { }

  iterator begin()
  {
    return iterator(pid_);
  }

  const_iterator begin() const
  {
    return const_iterator(pid_);
  }
  
  const_iterator cbegin() const
  {
    return const_iterator(pid_);
  }
  
  iterator end() HADESMEM_NOEXCEPT
  {
    return iterator();
  }

  const_iterator end() const HADESMEM_NOEXCEPT
  {
    return const_iterator();
  }
  
  const_iterator cend() const HADESMEM_NOEXCEPT
  {
    return const_iterator();
  }

private:
  DWORD pid_;
};

}
