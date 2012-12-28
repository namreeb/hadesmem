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

// ProcessIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ProcessIterator : public std::iterator<std::input_iterator_tag, 
  ProcessEntry>
{
public:
  ProcessIterator() HADESMEM_NOEXCEPT;

  explicit ProcessIterator(int dummy);

  ProcessIterator(ProcessIterator const& other) HADESMEM_NOEXCEPT;

  ProcessIterator& operator=(ProcessIterator const& other) HADESMEM_NOEXCEPT;

  ProcessIterator(ProcessIterator&& other) HADESMEM_NOEXCEPT;

  ProcessIterator& operator=(ProcessIterator&& other) HADESMEM_NOEXCEPT;

  ~ProcessIterator();
  
  reference operator*() const HADESMEM_NOEXCEPT;
  
  pointer operator->() const HADESMEM_NOEXCEPT;
  
  ProcessIterator& operator++();
  
  ProcessIterator operator++(int);
  
  bool operator==(ProcessIterator const& other) const HADESMEM_NOEXCEPT;
  
  bool operator!=(ProcessIterator const& other) const HADESMEM_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  struct Impl;
  std::shared_ptr<Impl> impl_;
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
