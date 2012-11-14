// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

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
