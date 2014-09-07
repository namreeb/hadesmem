// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <utility>
#include <functional>

#include <windows.h>

namespace hadesmem
{

namespace cerberus
{

typedef void OnUnloadCallbackD3D10();

void InitializeD3D10();

void DetourD3D10(HMODULE base);

void UndetourD3D10(bool remove);

void InitializeD3D101();

void DetourD3D101(HMODULE base);

void UndetourD3D101(bool remove);

std::size_t RegisterOnUnloadCallbackD3D10(
  std::function<OnUnloadCallbackD3D10> const& callback);

void UnregisterOnUnloadCallbackD3D10(std::size_t id);
}
}
