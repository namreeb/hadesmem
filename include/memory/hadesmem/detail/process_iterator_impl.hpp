// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <windows.h>

#include "hadesmem/config.hpp"
#include "hadesmem/process_entry.hpp"
#include "hadesmem/detail/optional.hpp"

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
  Optional<ProcessEntry> process_;
  
private:
  ProcessIteratorImpl(ProcessIteratorImpl const&) HADESMEM_DELETED_FUNCTION;
  ProcessIteratorImpl& operator=(ProcessIteratorImpl const&) 
    HADESMEM_DELETED_FUNCTION;
};

}

}
