// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>
#include <functional>

#include <windows.h>

#include <dxgi.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
typedef void OnFrameCallbackDXGI(IDXGISwapChain* swap_chain);

class DXGIInterface
{
public:
  virtual ~DXGIInterface()
  {
  }

  virtual std::size_t RegisterOnFrameCallback(
    std::function<OnFrameCallbackDXGI> const& callback) = 0;

  virtual void UnregisterOnFrameCallback(std::size_t id) = 0;
};

DXGIInterface& GetDXGIInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeDXGI();

void DetourDXGI(HMODULE base);

void UndetourDXGI(bool remove);

std::size_t RegisterOnFrameCallbackDXGI(
  std::function<OnFrameCallbackDXGI> const& callback);

void UnregisterOnFrameCallbackDXGI(std::size_t id);

void DetourDXGISwapChain(IDXGISwapChain* swap_chain);

void DetourDXGIFactory(IDXGIFactory* dxgi_factory);

void DetourDXGIFactoryFromDevice(IUnknown* device);
}
}
