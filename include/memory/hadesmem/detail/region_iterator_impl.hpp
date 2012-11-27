// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/optional.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/config.hpp"
#include "hadesmem/region.hpp"

namespace hadesmem
{

class Process;

namespace detail
{

struct RegionIteratorImpl
{
  RegionIteratorImpl() HADESMEM_NOEXCEPT
    : process_(nullptr), 
    region_()
  { }
  
  Process const* process_;
  boost::optional<Region> region_;
  
private:
  RegionIteratorImpl(RegionIteratorImpl const&) HADESMEM_DELETED_FUNCTION;
  RegionIteratorImpl& operator=(RegionIteratorImpl const&) 
    HADESMEM_DELETED_FUNCTION;
};

}

}
