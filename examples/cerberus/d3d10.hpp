// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <utility>

#include <windows.h>

namespace hadesmem
{

namespace cerberus
{

  void InitializeD3D10();

void DetourD3D10(HMODULE base);

void UndetourD3D10(bool remove);

void InitializeD3D101();

void DetourD3D101(HMODULE base);

void UndetourD3D101(bool remove);
}
}
