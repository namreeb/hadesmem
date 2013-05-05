// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <utility>
#include <iterator>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/optional.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/detail/smart_handle.hpp>

namespace hadesmem
{

class Process;

class ProcessEntry;

// ProcessIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ProcessIterator : public std::iterator<std::input_iterator_tag, 
  ProcessEntry>
{
public:
  ProcessIterator() HADESMEM_NOEXCEPT
    : impl_()
  { }

  explicit ProcessIterator(int dummy);

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
  
  ProcessIterator& operator++();
  
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
