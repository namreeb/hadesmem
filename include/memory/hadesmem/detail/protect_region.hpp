// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

namespace detail
{

DWORD Protect(Process const& process, MEMORY_BASIC_INFORMATION const& mbi, 
  DWORD protect);

}

}
