// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/config.hpp>
#include <boost/optional.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

#include "hadesmem/module.hpp"
#include "hadesmem/detail/smart_handle.hpp"

namespace hadesmem
{

class Process;

namespace detail
{

struct ModuleIteratorImpl
{
  ModuleIteratorImpl() BOOST_NOEXCEPT
    : process_(nullptr), 
    snap_(INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE), 
    module_()
  { }
  
  Process const* process_;
  SmartHandle snap_;
  boost::optional<Module> module_;
  
private:
  ModuleIteratorImpl(ModuleIteratorImpl const&);
  ModuleIteratorImpl& operator=(ModuleIteratorImpl const&);
};

}

}
