// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <utility>
#include <functional>

#include <windows.h>

#include <d3d11.h>

namespace hadesmem
{

namespace cerberus
{

void InitializeD3D11();

void DetourD3D11(HMODULE base);

void DetourDXGI(HMODULE base);

void UndetourD3D11(bool remove);

void UndetourDXGI(bool remove);

typedef void OnFrameCallbackD3D11(IDXGISwapChain* swap_chain);

std::size_t RegisterOnFrameCallbackD3D11(
  std::function<OnFrameCallbackD3D11> const& callback);

void UnregisterOnFrameCallbackD3D11(std::size_t id);
}
}
