// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <hadesmem/config.hpp>

#include "render.hpp"

namespace hadesmem
{
namespace cerberus
{
class GwenInterface;

typedef void OnGwenInitializeCallback(GwenInterface* ant_tweak_bar);

typedef void OnGwenCleanupCallback(GwenInterface* ant_tweak_bar);

class GwenInterface
{
public:
  virtual ~GwenInterface()
  {
  }

  virtual std::size_t RegisterOnInitialize(
    std::function<OnGwenInitializeCallback> const& callback) = 0;

  virtual void UnregisterOnInitialize(std::size_t id) = 0;

  virtual std::size_t
    RegisterOnCleanup(std::function<OnGwenCleanupCallback> const& callback) = 0;

  virtual void UnregisterOnCleanup(std::size_t id) = 0;

  virtual bool IsInitialized() = 0;
};

GwenInterface& GetGwenInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeGwen();
}
}
