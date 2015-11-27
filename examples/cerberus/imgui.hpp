// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
class ImguiInterface;

typedef void OnImguiInitializeCallback(ImguiInterface* imgui);

typedef void OnImguiCleanupCallback(ImguiInterface* imgui);

class ImguiInterface
{
public:
  virtual ~ImguiInterface()
  {
  }

  virtual std::size_t RegisterOnInitialize(
    std::function<OnImguiInitializeCallback> const& callback) = 0;

  virtual void UnregisterOnInitialize(std::size_t id) = 0;

  virtual std::size_t RegisterOnCleanup(
    std::function<OnImguiCleanupCallback> const& callback) = 0;

  virtual void UnregisterOnCleanup(std::size_t id) = 0;

  virtual bool IsInitialized() = 0;
};

ImguiInterface& GetImguiInterface() noexcept;

void InitializeImgui();
}
}
