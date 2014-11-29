// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>
#include <functional>

#include <windows.h>

#include <dxgi.h>

#include <hadesmem/config.hpp>

#if !defined(HADESMEM_DETAIL_NO_DXGI1_2)
#include <dxgi1_2.h>
#endif // #if !defined(HADESMEM_DETAIL_NO_DXGI1_2)

namespace hadesmem
{
namespace cerberus
{
typedef void OnFrameDXGICallback(IDXGISwapChain* swap_chain);

class DXGIInterface
{
public:
  virtual ~DXGIInterface()
  {
  }

  virtual std::size_t
    RegisterOnFrame(std::function<OnFrameDXGICallback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;
};

DXGIInterface& GetDXGIInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeDXGI();

void DetourDXGI(HMODULE base);

void UndetourDXGI(bool remove);

void DetourDXGISwapChain(IDXGISwapChain* swap_chain);

#if !defined(HADESMEM_DETAIL_NO_DXGI1_2)
void DetourDXGISwapChain(IDXGISwapChain1* swap_chain);
#endif // #if !defined(HADESMEM_DETAIL_NO_DXGI1_2)

void DetourDXGIFactory(IDXGIFactory* dxgi_factory);

#if !defined(HADESMEM_DETAIL_NO_DXGI1_2)
void DetourDXGIFactory(IDXGIFactory2* dxgi_factory);
#endif // #if !defined(HADESMEM_DETAIL_NO_DXGI1_2)

void DetourDXGIFactoryFromDevice(IUnknown* device);
}
}
