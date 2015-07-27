// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>
#include <functional>

#include <windows.h>

#include <hadesmem/config.hpp>

#include <dxgi1_3.h>
#include <dxgi.h>

#include "callbacks.hpp"

namespace hadesmem
{
namespace cerberus
{
typedef void OnFrameDXGICallback(IDXGISwapChain* swap_chain);

typedef void OnReleaseDXGICallback(IDXGISwapChain* swap_chain);

typedef void
  OnResizeDXGICallback(IDXGISwapChain* swap_chain, UINT width, UINT height);

Callbacks<OnFrameDXGICallback>& GetOnFrameDXGICallbacks();

Callbacks<OnReleaseDXGICallback>& GetOnReleaseDXGICallbacks();

Callbacks<OnResizeDXGICallback>& GetOnResizeDXGICallbacks();

class DXGIInterface
{
public:
  virtual ~DXGIInterface()
  {
  }

  virtual std::size_t
    RegisterOnFrame(std::function<OnFrameDXGICallback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;

  virtual std::size_t RegisterOnRelease(
    std::function<hadesmem::cerberus::OnReleaseDXGICallback> const&
      callback) = 0;

  virtual void UnregisterOnRelease(std::size_t id) = 0;

  virtual std::size_t RegisterOnResize(
    std::function<hadesmem::cerberus::OnResizeDXGICallback> const&
      callback) = 0;

  virtual void UnregisterOnResize(std::size_t id) = 0;
};

DXGIInterface& GetDXGIInterface() noexcept;

void InitializeDXGI();

void DetourDXGI(HMODULE base);

void UndetourDXGI(bool remove);
}
}
