// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

namespace hadesmem
{
namespace cerberus
{
void InitializeProcess();

void DetourKernelBaseForProcess(HMODULE base);

void UndetourKernelBaseForProcess(bool remove);
}
}
