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

class ImportDir;

// ImportDirIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ImportDirIterator : public std::iterator<std::input_iterator_tag, 
  ImportDir>
{
public:
  ImportDirIterator() HADESMEM_NOEXCEPT;
  
  explicit ImportDirIterator(Process const& process, PeFile const& pe_file);

  ImportDirIterator(ImportDirIterator const& other) HADESMEM_NOEXCEPT;

  ImportDirIterator& operator=(ImportDirIterator const& other) 
    HADESMEM_NOEXCEPT;

  ImportDirIterator(ImportDirIterator&& other) HADESMEM_NOEXCEPT;

  ImportDirIterator& operator=(ImportDirIterator&& other) HADESMEM_NOEXCEPT;

  ~ImportDirIterator();
  
  reference operator*() const HADESMEM_NOEXCEPT;
  
  pointer operator->() const HADESMEM_NOEXCEPT;
  
  ImportDirIterator& operator++();
  
  ImportDirIterator operator++(int);
  
  bool operator==(ImportDirIterator const& other) const HADESMEM_NOEXCEPT;
  
  bool operator!=(ImportDirIterator const& other) const HADESMEM_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

class ImportDirList
{
public:
  typedef ImportDir value_type;
  typedef ImportDirIterator iterator;
  typedef ImportDirIterator const_iterator;
  
  explicit ImportDirList(Process const& process, PeFile const& pe_file);

  ImportDirList(ImportDirList const& other);

  ImportDirList& operator=(ImportDirList const& other);

  ImportDirList(ImportDirList&& other) HADESMEM_NOEXCEPT;

  ImportDirList& operator=(ImportDirList&& other) HADESMEM_NOEXCEPT;

  ~ImportDirList();
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() HADESMEM_NOEXCEPT;
  
  const_iterator end() const HADESMEM_NOEXCEPT;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
