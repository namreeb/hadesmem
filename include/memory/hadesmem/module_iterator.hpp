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
#include <boost/iterator/iterator_facade.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include "hadesmem/module.hpp"

namespace hadesmem
{

class Process;

namespace detail
{
  struct ModuleIteratorImpl;
}

class ModuleIterator : 
  public boost::iterator_facade<
    ModuleIterator, 
    Module, 
    boost::single_pass_traversal_tag
    >
{
public:
  ModuleIterator() BOOST_NOEXCEPT;
  
  ModuleIterator(Process const& process);
  
  ~ModuleIterator() BOOST_NOEXCEPT;
  
private:
  friend class boost::iterator_core_access;
  
  typedef boost::iterator_facade<
    ModuleIterator, 
    Module, 
    boost::single_pass_traversal_tag
    > ModuleIteratorFacade;
  
  ModuleIteratorFacade::reference dereference() const;
  
  void increment();
  
  bool equal(ModuleIterator const& other) const BOOST_NOEXCEPT;
  
  std::shared_ptr<detail::ModuleIteratorImpl> impl_;
};

}
