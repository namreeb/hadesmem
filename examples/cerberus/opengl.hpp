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
typedef void OnFrameOpenGL32Callback(HDC device);

class OpenGL32Interface
{
public:
  virtual ~OpenGL32Interface()
  {
  }

  virtual std::size_t
    RegisterOnFrame(std::function<OnFrameOpenGL32Callback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;
};

OpenGL32Interface& GetOpenGL32Interface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeOpenGL32();

void DetourOpenGL32(HMODULE base);

void UndetourOpenGL32(bool remove);
}
}
