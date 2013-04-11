// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <iterator>

#include <hadesmem/config.hpp>

namespace hadesmem
{

class Process;

class PeFile;

class Export;

// ExportIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ExportIterator : public std::iterator<std::input_iterator_tag, Export>
{
public:
  ExportIterator() HADESMEM_NOEXCEPT;
  
  explicit ExportIterator(Process const& process, PeFile const& pe_file);

  ExportIterator(ExportIterator const& other) HADESMEM_NOEXCEPT;

  ExportIterator& operator=(ExportIterator const& other) HADESMEM_NOEXCEPT;

  ExportIterator(ExportIterator&& other) HADESMEM_NOEXCEPT;

  ExportIterator& operator=(ExportIterator&& other) HADESMEM_NOEXCEPT;

  ~ExportIterator();
  
  reference operator*() const HADESMEM_NOEXCEPT;
  
  pointer operator->() const HADESMEM_NOEXCEPT;
  
  ExportIterator& operator++();
  
  ExportIterator operator++(int);
  
  bool operator==(ExportIterator const& other) const HADESMEM_NOEXCEPT;
  
  bool operator!=(ExportIterator const& other) const HADESMEM_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

class ExportList
{
public:
  typedef Export value_type;
  typedef ExportIterator iterator;
  typedef ExportIterator const_iterator;
  
  explicit ExportList(Process const& process, PeFile const& pe_file);

  ExportList(ExportList const& other);

  ExportList& operator=(ExportList const& other);

  ExportList(ExportList&& other) HADESMEM_NOEXCEPT;

  ExportList& operator=(ExportList&& other) HADESMEM_NOEXCEPT;

  ~ExportList();
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() HADESMEM_NOEXCEPT;
  
  const_iterator end() const HADESMEM_NOEXCEPT;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
