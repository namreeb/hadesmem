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
typedef void OnFrameCallbackOGL(HDC device);

typedef void OnUnloadCallbackOGL();

class OGLInterface
{
public:
  virtual ~OGLInterface()
  {
  }

  virtual std::size_t RegisterOnFrameCallback(
    std::function<OnFrameCallbackOGL> const& callback) = 0;

  virtual void UnregisterOnFrameCallback(std::size_t id) = 0;

  virtual std::size_t RegisterOnUnloadCallback(
    std::function<OnUnloadCallbackOGL> const& callback) = 0;

  virtual void UnregisterOnUnloadCallback(std::size_t id) = 0;
};

OGLInterface& GetOGLInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeOGL();

void DetourOGL(HMODULE base);

void UndetourOGL(bool remove);

std::size_t RegisterOnFrameCallbackOGL(
  std::function<OnFrameCallbackOGL> const& callback);

void UnregisterOnFrameCallbackOGL(std::size_t id);

std::size_t RegisterOnUnloadCallbackOGL(
  std::function<OnUnloadCallbackOGL> const& callback);

void UnregisterOnUnloadCallbackOGL(std::size_t id);
}
}
