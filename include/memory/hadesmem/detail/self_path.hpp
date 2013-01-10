// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>

#include <windows.h>

namespace hadesmem
{

namespace detail
{

HMODULE GetHandleToSelf();

std::wstring GetSelfPath();

std::wstring GetSelfDirPath();

}

}
