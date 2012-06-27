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

class ProcessIterator;

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
