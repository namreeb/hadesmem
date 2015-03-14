// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

namespace hadesmem
{
namespace cerberus
{
void InitializeException();

void DetourNtdllForException(HMODULE base);

void DetourKernelBaseForException(HMODULE base);

void UndetourNtdllForException(bool remove);

void UndetourKernelBaseForException(bool remove);
}
}
