// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>
#include <functional>

#include <windows.h>

#include <dxgi1_2.h>
#include <dxgi.h>

#include <hadesmem/config.hpp>

#include "callbacks.hpp"

namespace hadesmem
{
namespace cerberus
{
typedef void OnFrameDXGICallback(IDXGISwapChain* swap_chain);

typedef void OnReleaseDXGICallback(IDXGISwapChain* swap_chain);

Callbacks<OnFrameDXGICallback>& GetOnFrameDXGICallbacks();

Callbacks<OnReleaseDXGICallback>& GetOnReleaseDXGICallbacks();

class DXGIInterface
{
public:
  virtual ~DXGIInterface()
  {
  }

  virtual std::size_t
    RegisterOnFrame(std::function<OnFrameDXGICallback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;

  virtual std::size_t RegisterOnRelease(std::function<
    hadesmem::cerberus::OnReleaseDXGICallback> const& callback) = 0;

  virtual void UnregisterOnRelease(std::size_t id) = 0;
};

DXGIInterface& GetDXGIInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeDXGI();

void DetourDXGI(HMODULE base);

void UndetourDXGI(bool remove);
}
}
