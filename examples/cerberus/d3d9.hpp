// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <utility>
#include <functional>

#include <windows.h>

#include <d3d9.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

namespace cerberus
{

typedef void OnFrameCallbackD3D9(IDirect3DDevice9* device);

class D3D9Interface
{
public:
  virtual ~D3D9Interface()
  {
  }

  virtual std::size_t RegisterOnFrameCallback(
    std::function<OnFrameCallbackD3D9> const& callback) = 0;

  virtual void UnregisterOnFrameCallback(std::size_t id) = 0;
};

D3D9Interface& GetD3D9Interface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeD3D9();

void DetourD3D9(HMODULE base);

void UndetourD3D9(bool remove);

std::size_t RegisterOnFrameCallbackD3D9(
  std::function<OnFrameCallbackD3D9> const& callback);

void UnregisterOnFrameCallbackD3D9(std::size_t id);
}
}
