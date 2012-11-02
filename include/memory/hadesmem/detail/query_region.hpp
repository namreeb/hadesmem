// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/config.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

namespace hadesmem
{

class Process;

namespace detail
{

MEMORY_BASIC_INFORMATION Query(Process const& process, LPCVOID address);

bool CanRead(MEMORY_BASIC_INFORMATION const& mbi) BOOST_NOEXCEPT;

bool CanWrite(MEMORY_BASIC_INFORMATION const& mbi) BOOST_NOEXCEPT;

bool CanExecute(MEMORY_BASIC_INFORMATION const& mbi) BOOST_NOEXCEPT;

bool IsGuard(MEMORY_BASIC_INFORMATION const& mbi) BOOST_NOEXCEPT;
}

}
