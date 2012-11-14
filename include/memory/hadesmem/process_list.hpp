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

// Inheriting from std::iterator causes the following warning under GCC:
// error: base class 'struct std::iterator<std::input_iterator_tag, 
// hadesmem::Process>' has a non-virtual destructor [-Werror=effc++]
// This can be ignored because iterators are not manipulated polymorphically.
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

// ModuleIterator satisfies the requirements of an input iterator 
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

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

class ProcessList
{
public:
  typedef ProcessIterator iterator;
  typedef ProcessIterator const_iterator;
  
  explicit ProcessList() HADESMEM_NOEXCEPT;
  
  ProcessList(ProcessList const& other) HADESMEM_NOEXCEPT;
  
  ProcessList& operator=(ProcessList const& other) HADESMEM_NOEXCEPT;
  
  ProcessList(ProcessList&& other) HADESMEM_NOEXCEPT;
  
  ProcessList& operator=(ProcessList&& other) HADESMEM_NOEXCEPT;
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() HADESMEM_NOEXCEPT;
  
  const_iterator end() const HADESMEM_NOEXCEPT;
};

}
