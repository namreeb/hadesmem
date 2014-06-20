// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <utility>
#include <functional>

#include <windows.h>

#include <d3d9.h>

namespace hadesmem
{

namespace cerberus
{

void InitializeD3D9();

void DetourD3D9(HMODULE base);

void UndetourD3D9(bool remove);

typedef void OnFrameCallbackD3D9(IDirect3DDevice9* device);

std::size_t RegisterOnFrameCallbackD3D9(
  std::function<OnFrameCallbackD3D9> const& callback);

void UnregisterOnFrameCallbackD3D9(std::size_t id);
}
}
