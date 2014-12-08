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
enum class RenderApi
{
  D3D9,
  D3D10,
  D3D11,
  OpenGL32
};

typedef void OnFrameCallback();

typedef void OnSetGuiVisibilityCallback(bool visibility, bool old_visibility);

typedef void OnInitializeGuiCallback(RenderApi api, void* device);

typedef void OnCleanupGuiCallback(RenderApi api);

class RenderInterface
{
public:
  virtual ~RenderInterface()
  {
  }

  virtual std::size_t
    RegisterOnFrame(std::function<OnFrameCallback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;

  virtual std::size_t RegisterOnSetGuiVisibility(
    std::function<OnSetGuiVisibilityCallback> const& callback) = 0;

  virtual void UnregisterOnSetGuiVisibility(std::size_t id) = 0;

  virtual std::size_t RegisterOnInitializeGui(
    std::function<OnInitializeGuiCallback> const& callback) = 0;

  virtual void UnregisterOnInitializeGui(std::size_t id) = 0;

  virtual std::size_t RegisterOnCleanupGui(
    std::function<OnCleanupGuiCallback> const& callback) = 0;

  virtual void UnregisterOnCleanupGui(std::size_t id) = 0;
};

RenderInterface& GetRenderInterface() HADESMEM_DETAIL_NOEXCEPT;

bool& GetGuiVisible() HADESMEM_DETAIL_NOEXCEPT;

void SetGuiVisible(bool visible, bool old_visible);

void InitializeRender();
}
}
