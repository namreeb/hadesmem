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
typedef void OnUnloadCallbackD3D11();

void InitializeD3D11();

void DetourD3D11(HMODULE base);

void UndetourD3D11(bool remove);

std::size_t RegisterOnUnloadCallbackD3D11(
  std::function<OnUnloadCallbackD3D11> const& callback);

void UnregisterOnUnloadCallbackD3D11(std::size_t id);
}
}
