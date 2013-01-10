// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>

#include <windows.h>

namespace hadesmem
{

class Process;

namespace detail
{

void Write(Process const& process, PVOID address, LPCVOID data, 
  std::size_t len);

}

}
