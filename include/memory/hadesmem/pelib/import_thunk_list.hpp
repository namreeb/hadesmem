// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <iterator>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

class PeFile;

class ImportThunk;

// ImportThunkIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ImportThunkIterator : public std::iterator<std::input_iterator_tag, 
  ImportThunk>
{
public:
  ImportThunkIterator() HADESMEM_NOEXCEPT;
  
  explicit ImportThunkIterator(Process const& process, PeFile const& pe_file, 
    DWORD first_thunk);

  ImportThunkIterator(ImportThunkIterator const& other) HADESMEM_NOEXCEPT;

  ImportThunkIterator& operator=(ImportThunkIterator const& other) 
    HADESMEM_NOEXCEPT;

  ImportThunkIterator(ImportThunkIterator&& other) HADESMEM_NOEXCEPT;

  ImportThunkIterator& operator=(ImportThunkIterator&& other) 
    HADESMEM_NOEXCEPT;

  ~ImportThunkIterator();
  
  reference operator*() const HADESMEM_NOEXCEPT;
  
  pointer operator->() const HADESMEM_NOEXCEPT;
  
  ImportThunkIterator& operator++();
  
  ImportThunkIterator operator++(int);
  
  bool operator==(ImportThunkIterator const& other) const HADESMEM_NOEXCEPT;
  
  bool operator!=(ImportThunkIterator const& other) const HADESMEM_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

class ImportThunkList
{
public:
  typedef ImportThunk value_type;
  typedef ImportThunkIterator iterator;
  typedef ImportThunkIterator const_iterator;
  
  explicit ImportThunkList(Process const& process, PeFile const& pe_file, 
    DWORD first_thunk);

  ImportThunkList(ImportThunkList const& other);

  ImportThunkList& operator=(ImportThunkList const& other);

  ImportThunkList(ImportThunkList&& other) HADESMEM_NOEXCEPT;

  ImportThunkList& operator=(ImportThunkList&& other) HADESMEM_NOEXCEPT;

  ~ImportThunkList();
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() HADESMEM_NOEXCEPT;
  
  const_iterator end() const HADESMEM_NOEXCEPT;
  
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
