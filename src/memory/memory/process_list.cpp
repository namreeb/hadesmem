// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/process_list.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/process_iterator.hpp"

namespace hadesmem
{

ProcessList::ProcessList() BOOST_NOEXCEPT
{ }

ProcessList::ProcessList(ProcessList const& /*other*/) BOOST_NOEXCEPT
{ }

ProcessList& ProcessList::operator=(ProcessList const& /*other*/) 
  BOOST_NOEXCEPT
{
  return *this;
}

ProcessList::ProcessList(ProcessList&& /*other*/) BOOST_NOEXCEPT
{ }

ProcessList& ProcessList::operator=(ProcessList&& /*other*/) BOOST_NOEXCEPT
{
  return *this;
}

ProcessList::~ProcessList()
{ }

ProcessList::iterator ProcessList::begin()
{
  return ProcessList::iterator(0);
}

ProcessList::const_iterator ProcessList::begin() const
{
  return ProcessList::iterator(0);
}

ProcessList::iterator ProcessList::end() BOOST_NOEXCEPT
{
  return ProcessList::iterator();
}

ProcessList::const_iterator ProcessList::end() const BOOST_NOEXCEPT
{
  return ProcessList::iterator();
}

}
