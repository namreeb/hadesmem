// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>
#include <functional>

#include <windows.h>

#include <dxgi.h>
#include <dxgi1_3.h>

namespace hadesmem
{

namespace cerberus
{

void InitializeDXGI();

void DetourDXGI(HMODULE base);

void UndetourDXGI(bool remove);

typedef void OnFrameCallbackDXGI(IDXGISwapChain* swap_chain);

std::size_t RegisterOnFrameCallbackDXGI(
  std::function<OnFrameCallbackDXGI> const& callback);

void UnregisterOnFrameCallbackDXGI(std::size_t id);

void DetourDXGISwapChainByRevision(IDXGISwapChain* swap_chain,
                                   std::uint32_t revision);

void DetourDXGIFactoryByRevision(IDXGIFactory* dxgi_factory,
                                 std::uint32_t revision);
}
}
