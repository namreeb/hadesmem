// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <utility>
#include <functional>

#include <windows.h>

#include <d3d9.h>

#include <hadesmem/config.hpp>

#include "callbacks.hpp"

// TODO: Clean this up. Use a function to generate and return the full name
// including the PID?
#define CERBERUS_HELPER_D3D9_MAP_NAME L"Local\\CerberusHelper_D3D9_"

namespace hadesmem
{
namespace cerberus
{
struct D3D9Offsets
{
  std::uintptr_t add_ref_;
  std::uintptr_t release_;
  std::uintptr_t present_;
  std::uintptr_t reset_;
  std::uintptr_t end_scene_;
  std::uintptr_t present_ex_;
  std::uintptr_t reset_ex_;
  std::uintptr_t swap_chain_present_;
};

typedef void OnFrameD3D9Callback(IDirect3DDevice9* device);

typedef void
  OnResetD3D9Callback(IDirect3DDevice9* device,
                      D3DPRESENT_PARAMETERS* presentation_parameters);

typedef void OnReleaseD3D9Callback(IDirect3DDevice9* device);

Callbacks<OnFrameD3D9Callback>& GetOnFrameD3D9Callbacks();

Callbacks<OnResetD3D9Callback>& GetOnResetD3D9Callbacks();

Callbacks<OnReleaseD3D9Callback>& GetOnReleaseD3D9Callbacks();

class D3D9Interface
{
public:
  virtual ~D3D9Interface()
  {
  }

  virtual std::size_t
    RegisterOnFrame(std::function<OnFrameD3D9Callback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;

  virtual std::size_t
    RegisterOnReset(std::function<OnResetD3D9Callback> const& callback) = 0;

  virtual void UnregisterOnReset(std::size_t id) = 0;

  virtual std::size_t
    RegisterOnRelease(std::function<OnReleaseD3D9Callback> const& callback) = 0;

  virtual void UnregisterOnRelease(std::size_t id) = 0;
};

D3D9Interface& GetD3D9Interface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeD3D9();

void DetourD3D9(HMODULE base);

void UndetourD3D9(bool remove);
}
}
