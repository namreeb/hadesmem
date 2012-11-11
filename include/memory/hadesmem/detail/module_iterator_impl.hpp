// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <windows.h>

#include "hadesmem/config.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/detail/optional.hpp"
#include "hadesmem/detail/smart_handle.hpp"

namespace hadesmem
{

class Process;

namespace detail
{

struct ModuleIteratorImpl
{
  ModuleIteratorImpl() HADESMEM_NOEXCEPT
    : process_(nullptr), 
    snap_(INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE), 
    module_()
  { }
  
  Process const* process_;
  SmartHandle snap_;
  Optional<Module> module_;
  
private:
  ModuleIteratorImpl(ModuleIteratorImpl const&) HADESMEM_DELETED_FUNCTION;
  ModuleIteratorImpl& operator=(ModuleIteratorImpl const&) 
    HADESMEM_DELETED_FUNCTION;
};

}

}
