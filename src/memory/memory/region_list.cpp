// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/region_list.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/region_iterator.hpp"

namespace hadesmem
{

RegionList::RegionList(Process const* process) BOOST_NOEXCEPT
  : process_(process)
{ }

RegionList::RegionList(RegionList const& other) BOOST_NOEXCEPT
  : process_(other.process_)
{ }

RegionList& RegionList::operator=(RegionList const& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  
  return *this;
}

RegionList::RegionList(RegionList&& other) BOOST_NOEXCEPT
  : process_(other.process_)
{
  other.process_ = nullptr;
}

RegionList& RegionList::operator=(RegionList&& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  
  other.process_ = nullptr;
  
  return *this;
}

RegionList::~RegionList()
{ }

RegionList::iterator RegionList::begin()
{
  return RegionList::iterator(process_);
}

RegionList::const_iterator RegionList::begin() const
{
  return RegionList::iterator(process_);
}

RegionList::iterator RegionList::end() BOOST_NOEXCEPT
{
  return RegionList::iterator();
}

RegionList::const_iterator RegionList::end() const BOOST_NOEXCEPT
{
  return RegionList::iterator();
}

}
