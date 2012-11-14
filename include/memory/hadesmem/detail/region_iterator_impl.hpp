// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include "hadesmem/config.hpp"
#include "hadesmem/region.hpp"
#include "hadesmem/detail/optional.hpp"

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
  Optional<Region> region_;
  
private:
  RegionIteratorImpl(RegionIteratorImpl const&) HADESMEM_DELETED_FUNCTION;
  RegionIteratorImpl& operator=(RegionIteratorImpl const&) 
    HADESMEM_DELETED_FUNCTION;
};

}

}
