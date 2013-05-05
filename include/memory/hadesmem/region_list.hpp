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

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/protect.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/query_region.hpp>

namespace hadesmem
{

// RegionIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class RegionIterator : public std::iterator<std::input_iterator_tag, Region>
{
public:
  RegionIterator() HADESMEM_NOEXCEPT
    : impl_()
  { }
  
  explicit RegionIterator(Process const& process)
    : impl_(new Impl(process))
  { }

  RegionIterator(RegionIterator const& other) HADESMEM_NOEXCEPT
    : impl_(other.impl_)
  { }

  RegionIterator& operator=(RegionIterator const& other) HADESMEM_NOEXCEPT
  {
    impl_ = other.impl_;

    return *this;
  }

  RegionIterator(RegionIterator&& other) HADESMEM_NOEXCEPT
    : impl_(std::move(other.impl_))
  { }

  RegionIterator& operator=(RegionIterator&& other) HADESMEM_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

  ~RegionIterator() HADESMEM_NOEXCEPT
  { }
  
  reference operator*() const HADESMEM_NOEXCEPT
  {
    HADESMEM_ASSERT(impl_.get());
    return *impl_->region_;
  }
  
  pointer operator->() const HADESMEM_NOEXCEPT
  {
    HADESMEM_ASSERT(impl_.get());
    return &*impl_->region_;
  }
  
  RegionIterator& operator++()
  {
    try
    {
      HADESMEM_ASSERT(impl_.get());
    
      void const* const base = impl_->region_->GetBase();
      SIZE_T const size = impl_->region_->GetSize();
      auto const next = static_cast<char const* const>(base) + size;
      MEMORY_BASIC_INFORMATION const mbi = detail::Query(*impl_->process_, next);
      impl_->region_ = Region(*impl_->process_, mbi);
    }
    catch (std::exception const& /*e*/)
    {
      // TODO: Check whether this is the right thing to do. We should only 
      // flag as the 'end' once we've actually reached the end of the list. If 
      // the iteration fails we should throw an exception.
      impl_.reset();
    }
  
    return *this;
  }
  
  RegionIterator operator++(int)
  {
    RegionIterator iter(*this);
    ++*this;
    return iter;
  }

  
  bool operator==(RegionIterator const& other) const HADESMEM_NOEXCEPT
  {
    return impl_ == other.impl_;
  }
  
  bool operator!=(RegionIterator const& other) const HADESMEM_NOEXCEPT
  {
    return !(*this == other);
  }
  
private:
  struct Impl
  {
    explicit Impl(Process const& process) HADESMEM_NOEXCEPT
      : process_(&process), 
      region_()
    {
      MEMORY_BASIC_INFORMATION const mbi = detail::Query(process, nullptr);
      region_ = Region(process, mbi);
    }

    Process const* process_;
    boost::optional<Region> region_;
  };

  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class RegionList
{
public:
  typedef Region value_type;
  typedef RegionIterator iterator;
  typedef RegionIterator const_iterator;
  
  explicit RegionList(Process const& process)
    : process_(&process)
  { }

  RegionList(RegionList const& other) HADESMEM_NOEXCEPT
    : process_(other.process_)
  { }

  RegionList& operator=(RegionList const& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;

    return *this;
  }

  RegionList(RegionList&& other) HADESMEM_NOEXCEPT
    : process_(other.process_)
  { }

  RegionList& operator=(RegionList&& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;

    return *this;
  }

  ~RegionList() HADESMEM_NOEXCEPT
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
