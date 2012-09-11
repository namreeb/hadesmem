// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <memory>
#include <iterator>

#include <boost/config.hpp>

namespace hadesmem
{

class Process;

struct ProcessEntry;

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
  ProcessIterator() BOOST_NOEXCEPT;
  
  ProcessIterator(ProcessIterator const& other) BOOST_NOEXCEPT;
  
  ProcessIterator& operator=(ProcessIterator const& other) BOOST_NOEXCEPT;
  
  ProcessIterator(ProcessIterator&& other) BOOST_NOEXCEPT;
  
  ProcessIterator& operator=(ProcessIterator&& other) BOOST_NOEXCEPT;
  
  ~ProcessIterator();
  
  explicit ProcessIterator(int dummy);
  
  reference operator*() const BOOST_NOEXCEPT;
  
  pointer operator->() const BOOST_NOEXCEPT;
  
  ProcessIterator& operator++();
  
  ProcessIterator operator++(int);
  
  bool operator==(ProcessIterator const& other) const BOOST_NOEXCEPT;
  
  bool operator!=(ProcessIterator const& other) const BOOST_NOEXCEPT;
  
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
  
  explicit ProcessList() BOOST_NOEXCEPT;
  
  ProcessList(ProcessList const& other) BOOST_NOEXCEPT;
  
  ProcessList& operator=(ProcessList const& other) BOOST_NOEXCEPT;
  
  ProcessList(ProcessList&& other) BOOST_NOEXCEPT;
  
  ProcessList& operator=(ProcessList&& other) BOOST_NOEXCEPT;
  
  ~ProcessList();
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() BOOST_NOEXCEPT;
  
  const_iterator end() const BOOST_NOEXCEPT;
};

}
