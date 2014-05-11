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

typedef void OnFrameCallback(IDXGISwapChain* swap_chain);

std::size_t
  RegisterOnFrameCallback(std::function<OnFrameCallback> const& callback);

void UnregisterOnFrameCallback(std::size_t id);
}
}
