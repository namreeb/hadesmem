// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <string>

namespace hadesmem
{

namespace detail
{

// This routine appends the given argument to a command line such
// that CommandLineToArgvW will return the argument string unchanged.
// Arguments in a command line should be separated by spaces; this
// function does not add these spaces.
void ArgvQuote(std::wstring* command_line, std::wstring const& argument, 
  bool force);

}

}
