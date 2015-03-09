// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <utility>
#include <functional>

#include <windows.h>

#include <d3d10_1.h>
#include <d3d10.h>

#include "callbacks.hpp"

namespace hadesmem
{
namespace cerberus
{
typedef void OnReleaseD3D10Callback(ID3D10Device* device);

Callbacks<OnReleaseD3D10Callback>& GetOnReleaseD3D10Callbacks();

class D3D10Interface
{
public:
  virtual ~D3D10Interface()
  {
  }

  virtual std::size_t RegisterOnRelease(
    std::function<OnReleaseD3D10Callback> const& callback) = 0;

  virtual void UnregisterOnRelease(std::size_t id) = 0;
};

D3D10Interface& GetD3D10Interface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeD3D10();

void DetourD3D10(HMODULE base);

void UndetourD3D10(bool remove);

void InitializeD3D101();

void DetourD3D101(HMODULE base);

void UndetourD3D101(bool remove);
}
}
