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

#include "hadesmem/process_entry.hpp"

namespace hadesmem
{

namespace detail
{

struct ProcessIteratorImpl
{
  ProcessIteratorImpl() BOOST_NOEXCEPT
    : snap_(INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE), 
    process_()
  { }
  
  SmartHandle snap_;
  boost::optional<ProcessEntry> process_;
  
private:
  ProcessIteratorImpl(ProcessIteratorImpl const&);
  ProcessIteratorImpl& operator=(ProcessIteratorImpl const&);
};

}

}
