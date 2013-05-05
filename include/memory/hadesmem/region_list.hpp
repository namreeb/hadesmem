// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <iterator>

#include <hadesmem/config.hpp>

namespace hadesmem
{

class Process;

class Region;

// RegionIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class RegionIterator : public std::iterator<std::input_iterator_tag, Region>
{
public:
  RegionIterator() HADESMEM_NOEXCEPT;
  
  explicit RegionIterator(Process const& process);

  RegionIterator(RegionIterator const& other) HADESMEM_NOEXCEPT;

  RegionIterator& operator=(RegionIterator const& other) HADESMEM_NOEXCEPT;

  RegionIterator(RegionIterator&& other) HADESMEM_NOEXCEPT;

  RegionIterator& operator=(RegionIterator&& other) HADESMEM_NOEXCEPT;

  ~RegionIterator();
  
  reference operator*() const HADESMEM_NOEXCEPT;
  
  pointer operator->() const HADESMEM_NOEXCEPT;
  
  RegionIterator& operator++();
  
  RegionIterator operator++(int);
  
  bool operator==(RegionIterator const& other) const HADESMEM_NOEXCEPT;
  
  bool operator!=(RegionIterator const& other) const HADESMEM_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  struct Impl;
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
