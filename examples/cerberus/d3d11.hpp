// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <utility>

#include <windows.h>

namespace hadesmem
{

namespace cerberus
{

void InitializeD3D11();

void DetourD3D11(HMODULE base);

void UndetourD3D11(bool remove);
}
}
