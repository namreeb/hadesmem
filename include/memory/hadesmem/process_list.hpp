// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <iterator>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class ProcessEntry;

namespace detail
{

struct ProcessIteratorImpl;

}

// ProcessIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ProcessIterator : public std::iterator<std::input_iterator_tag, 
  ProcessEntry>
{
public:
  ProcessIterator() HADESMEM_NOEXCEPT;

  explicit ProcessIterator(int dummy);
  
  reference operator*() const HADESMEM_NOEXCEPT;
  
  pointer operator->() const HADESMEM_NOEXCEPT;
  
  ProcessIterator& operator++();
  
  ProcessIterator operator++(int);
  
  bool operator==(ProcessIterator const& other) const HADESMEM_NOEXCEPT;
  
  bool operator!=(ProcessIterator const& other) const HADESMEM_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<detail::ProcessIteratorImpl> impl_;
};

class ProcessList
{
public:
  typedef ProcessIterator iterator;
  typedef ProcessIterator const_iterator;

  ProcessList() HADESMEM_NOEXCEPT;
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() HADESMEM_NOEXCEPT;
  
  const_iterator end() const HADESMEM_NOEXCEPT;
};

}
