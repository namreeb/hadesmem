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
typedef void OnFrameCallbackOpenGL32(HDC device);

typedef void OnUnloadCallbackOpenGL32();

class OpenGL32Interface
{
public:
  virtual ~OpenGL32Interface()
  {
  }

  virtual std::size_t RegisterOnFrameCallback(
    std::function<OnFrameCallbackOpenGL32> const& callback) = 0;

  virtual void UnregisterOnFrameCallback(std::size_t id) = 0;

  virtual std::size_t RegisterOnUnloadCallback(
    std::function<OnUnloadCallbackOpenGL32> const& callback) = 0;

  virtual void UnregisterOnUnloadCallback(std::size_t id) = 0;
};

OpenGL32Interface& GetOpenGL32Interface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeOpenGL32();

void DetourOpenGL32(HMODULE base);

void UndetourOpenGL32(bool remove);

std::size_t RegisterOnFrameCallbackOpenGL32(
  std::function<OnFrameCallbackOpenGL32> const& callback);

void UnregisterOnFrameCallbackOpenGL32(std::size_t id);

std::size_t RegisterOnUnloadCallbackOpenGL32(
  std::function<OnUnloadCallbackOpenGL32> const& callback);

void UnregisterOnUnloadCallbackOpenGL32(std::size_t id);
}
}
