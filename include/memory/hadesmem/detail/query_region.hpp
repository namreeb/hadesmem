// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process;

namespace detail
{

MEMORY_BASIC_INFORMATION Query(Process const& process, LPCVOID address);

bool CanRead(MEMORY_BASIC_INFORMATION const& mbi) HADESMEM_NOEXCEPT;

bool CanWrite(MEMORY_BASIC_INFORMATION const& mbi) HADESMEM_NOEXCEPT;

bool CanExecute(MEMORY_BASIC_INFORMATION const& mbi) HADESMEM_NOEXCEPT;

bool IsGuard(MEMORY_BASIC_INFORMATION const& mbi) HADESMEM_NOEXCEPT;
}

}
