// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

namespace hadesmem
{

class Process;

bool CanRead(Process const& process, LPCVOID address);

bool CanWrite(Process const& process, LPCVOID address);

bool CanExecute(Process const& process, LPCVOID address);

bool IsGuard(Process const& process, LPCVOID address);

DWORD Protect(Process const& process, LPVOID address, DWORD protect);

}
