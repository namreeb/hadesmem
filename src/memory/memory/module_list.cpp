// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/module_list.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/module_iterator.hpp"

namespace hadesmem
{

ModuleList::ModuleList(Process const* process) BOOST_NOEXCEPT
  : process_(process)
{ }

ModuleList::ModuleList(ModuleList const& other) BOOST_NOEXCEPT
  : process_(other.process_)
{ }

ModuleList& ModuleList::operator=(ModuleList const& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  
  return *this;
}

ModuleList::ModuleList(ModuleList&& other) BOOST_NOEXCEPT
  : process_(other.process_)
{
  other.process_ = nullptr;
}

ModuleList& ModuleList::operator=(ModuleList&& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  
  other.process_ = nullptr;
  
  return *this;
}

ModuleList::~ModuleList()
{ }

ModuleList::iterator ModuleList::begin()
{
  return ModuleList::iterator(process_);
}

ModuleList::const_iterator ModuleList::begin() const
{
  return ModuleList::iterator(process_);
}

ModuleList::iterator ModuleList::end() BOOST_NOEXCEPT
{
  return ModuleList::iterator();
}

ModuleList::const_iterator ModuleList::end() const BOOST_NOEXCEPT
{
  return ModuleList::iterator();
}

}
