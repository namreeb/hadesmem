// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
class AntTweakBarInterface;

typedef void OnFrameCallback();

class RenderInterface
{
public:
  virtual ~RenderInterface()
  {
  }

  virtual std::size_t
    RegisterOnFrame(std::function<OnFrameCallback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;

  virtual AntTweakBarInterface* GetAntTweakBarInterface() = 0;
};

RenderInterface& GetRenderInterface() HADESMEM_DETAIL_NOEXCEPT;

bool AntTweakBarInitializedAny() HADESMEM_DETAIL_NOEXCEPT;

void InitializeRender();

}
}
