// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <memory>
#include <iterator>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/config.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

namespace hadesmem
{

class Process;

class Module;

namespace detail
{

struct ModuleIteratorImpl;

}

// Boost.Iterator causes the following warning under GCC:
// error: base class 'class boost::iterator_facade<hadesmem::ModuleIterator, 
// hadesmem::Module, boost::single_pass_traversal_tag>' has a non-virtual 
// destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

// ModuleIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
class ModuleIterator : public std::iterator<std::input_iterator_tag, Module>
{
public:
  ModuleIterator() BOOST_NOEXCEPT;
  
  ModuleIterator(Process const& process);
  
  reference operator*() const BOOST_NOEXCEPT;
  
  pointer operator->() const BOOST_NOEXCEPT;
  
  ModuleIterator& operator++();
  
  ModuleIterator operator++(int);
  
  bool operator==(ModuleIterator const& other) BOOST_NOEXCEPT;
  
  bool operator!=(ModuleIterator const& other) BOOST_NOEXCEPT;
  
private:
  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<detail::ModuleIteratorImpl> impl_;
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

}
