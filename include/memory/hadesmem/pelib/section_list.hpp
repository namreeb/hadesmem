// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <iterator>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class PeFile;

class Section;

// SectionIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class SectionIterator : public std::iterator<std::input_iterator_tag, Section>
{
public:
  SectionIterator() HADESMEM_NOEXCEPT;
  
  explicit SectionIterator(Process const& process, PeFile const& pe_file);

  SectionIterator(SectionIterator const& other) HADESMEM_NOEXCEPT;

  SectionIterator& operator=(SectionIterator const& other) HADESMEM_NOEXCEPT;

  SectionIterator(SectionIterator&& other) HADESMEM_NOEXCEPT;

  SectionIterator& operator=(SectionIterator&& other) HADESMEM_NOEXCEPT;

  ~SectionIterator();
  
  reference operator*() const HADESMEM_NOEXCEPT;
  
  pointer operator->() const HADESMEM_NOEXCEPT;
  
  SectionIterator& operator++();
  
  SectionIterator operator++(int);
  
  bool operator==(SectionIterator const& other) const HADESMEM_NOEXCEPT;
  
  bool operator!=(SectionIterator const& other) const HADESMEM_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

class SectionList
{
public:
  typedef Section value_type;
  typedef SectionIterator iterator;
  typedef SectionIterator const_iterator;
  
  explicit SectionList(Process const& process, PeFile const& pe_file);

  SectionList(SectionList const& other);

  SectionList& operator=(SectionList const& other);

  SectionList(SectionList&& other) HADESMEM_NOEXCEPT;

  SectionList& operator=(SectionList&& other) HADESMEM_NOEXCEPT;

  ~SectionList();
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() HADESMEM_NOEXCEPT;
  
  const_iterator end() const HADESMEM_NOEXCEPT;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
