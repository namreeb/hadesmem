// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>
#include <functional>

#include <windows.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

namespace cerberus
{

typedef void OnFrameCallback(class RenderInterface* render);

class RenderInterface
{
public:
  virtual ~RenderInterface()
  {
  }

  virtual std::size_t
    RegisterOnFrame(std::function<OnFrameCallback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;

  virtual void DrawWatermark() = 0;
};

RenderInterface& GetRenderInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeRender();

std::size_t
  RegisterOnFrameCallback(std::function<OnFrameCallback> const& callback);

void UnregisterOnFrameCallback(std::size_t id);
}
}
