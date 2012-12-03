// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <iterator>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class Module;

namespace detail
{

struct ModuleIteratorImpl;

}

// ModuleIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ModuleIterator : public std::iterator<std::input_iterator_tag, Module>
{
public:
  ModuleIterator() HADESMEM_NOEXCEPT;
  
  explicit ModuleIterator(Process const* process);
  
  reference operator*() const HADESMEM_NOEXCEPT;
  
  pointer operator->() const HADESMEM_NOEXCEPT;
  
  ModuleIterator& operator++();
  
  ModuleIterator operator++(int);
  
  bool operator==(ModuleIterator const& other) const HADESMEM_NOEXCEPT;
  
  bool operator!=(ModuleIterator const& other) const HADESMEM_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<detail::ModuleIteratorImpl> impl_;
};

class ModuleList
{
public:
  typedef Module value_type;
  typedef ModuleIterator iterator;
  typedef ModuleIterator const_iterator;
  
  explicit ModuleList(Process const* process) HADESMEM_NOEXCEPT;
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() HADESMEM_NOEXCEPT;
  
  const_iterator end() const HADESMEM_NOEXCEPT;
  
private:
  Process const* process_;
};

}
