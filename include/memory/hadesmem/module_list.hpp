// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <iterator>

#include <hadesmem/config.hpp>

namespace hadesmem
{

class Process;

class Module;

// ModuleIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ModuleIterator : public std::iterator<std::input_iterator_tag, Module>
{
public:
  ModuleIterator() HADESMEM_NOEXCEPT;
  
  explicit ModuleIterator(Process const& process);

  ModuleIterator(ModuleIterator const& other) HADESMEM_NOEXCEPT;

  ModuleIterator& operator=(ModuleIterator const& other) HADESMEM_NOEXCEPT;

  ModuleIterator(ModuleIterator&& other) HADESMEM_NOEXCEPT;

  ModuleIterator& operator=(ModuleIterator&& other) HADESMEM_NOEXCEPT;

  ~ModuleIterator();
  
  reference operator*() const HADESMEM_NOEXCEPT;
  
  pointer operator->() const HADESMEM_NOEXCEPT;
  
  ModuleIterator& operator++();
  
  ModuleIterator operator++(int);
  
  bool operator==(ModuleIterator const& other) const HADESMEM_NOEXCEPT;
  
  bool operator!=(ModuleIterator const& other) const HADESMEM_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

class ModuleList
{
public:
  typedef Module value_type;
  typedef ModuleIterator iterator;
  typedef ModuleIterator const_iterator;
  
  explicit ModuleList(Process const& process);

  ModuleList(ModuleList const& other);

  ModuleList& operator=(ModuleList const& other);

  ModuleList(ModuleList&& other) HADESMEM_NOEXCEPT;

  ModuleList& operator=(ModuleList&& other) HADESMEM_NOEXCEPT;

  ~ModuleList();
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() HADESMEM_NOEXCEPT;
  
  const_iterator end() const HADESMEM_NOEXCEPT;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
