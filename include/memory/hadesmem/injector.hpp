// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <string>

#include <windows.h>

namespace hadesmem
{

class Process;

// TODO: Overload bitwise operators for InjectFlags.
enum class InjectFlags
{
  None = 0, 
  PathResolution = 1 << 0, 
  InvalidFlagMaxValue = 1 << 1
};


HMODULE InjectDll(Process const& process, std::wstring const& path, 
  InjectFlags flags);

void FreeDll(Process const& process, HMODULE module);

// TODO: Add CreateAndInject functionality.


}
