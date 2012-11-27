// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/optional.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/config.hpp"
#include "hadesmem/process_entry.hpp"

namespace hadesmem
{

namespace detail
{

struct ProcessIteratorImpl
{
  ProcessIteratorImpl() HADESMEM_NOEXCEPT
    : snap_(INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE), 
    process_()
  { }
  
  SmartHandle snap_;
  boost::optional<ProcessEntry> process_;
  
private:
  ProcessIteratorImpl(ProcessIteratorImpl const&) HADESMEM_DELETED_FUNCTION;
  ProcessIteratorImpl& operator=(ProcessIteratorImpl const&) 
    HADESMEM_DELETED_FUNCTION;
};

}

}
