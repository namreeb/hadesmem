// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <iterator>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class Region;

namespace detail
{

struct RegionIteratorImpl;

}

// RegionIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class RegionIterator : public std::iterator<std::input_iterator_tag, Region>
{
public:
  RegionIterator() HADESMEM_NOEXCEPT;
  
  explicit RegionIterator(Process const* process);
  
  reference operator*() const HADESMEM_NOEXCEPT;
  
  pointer operator->() const HADESMEM_NOEXCEPT;
  
  RegionIterator& operator++();
  
  RegionIterator operator++(int);
  
  bool operator==(RegionIterator const& other) const HADESMEM_NOEXCEPT;
  
  bool operator!=(RegionIterator const& other) const HADESMEM_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<detail::RegionIteratorImpl> impl_;
};

class RegionList
{
public:
  typedef Region value_type;
  typedef RegionIterator iterator;
  typedef RegionIterator const_iterator;
  
  explicit RegionList(Process const* process) HADESMEM_NOEXCEPT;
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() HADESMEM_NOEXCEPT;
  
  const_iterator end() const HADESMEM_NOEXCEPT;
  
private:
  Process const* process_;
};

}
