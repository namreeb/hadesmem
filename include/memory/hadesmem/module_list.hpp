// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/config.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

namespace hadesmem
{

class Process;

class Module;

class ModuleIterator;

class ModuleList
{
public:
  typedef Module value_type;
  typedef ModuleIterator iterator;
  typedef ModuleIterator const_iterator;
  
  explicit ModuleList(Process const* process) BOOST_NOEXCEPT;
  
  ModuleList(ModuleList const& other) BOOST_NOEXCEPT;
  
  ModuleList& operator=(ModuleList const& other) BOOST_NOEXCEPT;
  
  ModuleList(ModuleList&& other) BOOST_NOEXCEPT;
  
  ModuleList& operator=(ModuleList&& other) BOOST_NOEXCEPT;
  
  ~ModuleList();
  
  iterator begin();
  
  const_iterator begin() const;
  
  iterator end() BOOST_NOEXCEPT;
  
  const_iterator end() const BOOST_NOEXCEPT;
  
private:
  Process const* process_;
};

}
