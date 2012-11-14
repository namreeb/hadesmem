// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>

#include <windows.h>

namespace hadesmem
{

class Process;

namespace detail
{

void Read(Process const& process, LPVOID address, LPVOID data, 
  std::size_t len);

void ReadUnchecked(Process const& process, LPVOID address, LPVOID data, 
  std::size_t len);

}

}
