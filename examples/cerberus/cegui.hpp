// Copyright (C) 2010-2015 Joshua Boyce
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
class CeguiInterface;

typedef void OnCeguiInitializeCallback(CeguiInterface* ant_tweak_bar);

typedef void OnCeguiCleanupCallback(CeguiInterface* ant_tweak_bar);

class CeguiInterface
{
public:
  virtual ~CeguiInterface()
  {
  }

  virtual std::size_t RegisterOnInitialize(
    std::function<OnCeguiInitializeCallback> const& callback) = 0;

  virtual void UnregisterOnInitialize(std::size_t id) = 0;

  virtual std::size_t RegisterOnCleanup(
    std::function<OnCeguiCleanupCallback> const& callback) = 0;

  virtual void UnregisterOnCleanup(std::size_t id) = 0;

  virtual bool IsInitialized() = 0;
};

CeguiInterface& GetCeguiInterface() noexcept;

void InitializeCegui();
}
}
