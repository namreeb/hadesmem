// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <windows.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
enum class RenderApi
{
  kD3D9,
  kD3D10,
  kD3D11,
  kOpenGL32,
  kInvalidMaxValue
};

typedef void OnFrameCallback(RenderApi api, void* device);

typedef void
  OnResizeCallback(RenderApi api, void* device, UINT width, UINT height);

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

  virtual std::size_t
    RegisterOnFrame2(std::function<OnFrameCallback> const& callback) = 0;

  virtual void UnregisterOnFrame2(std::size_t id) = 0;

  virtual std::size_t
    RegisterOnResize(std::function<OnResizeCallback> const& callback) = 0;

  virtual void UnregisterOnResize(std::size_t id) = 0;

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

RenderInterface& GetRenderInterface() noexcept;

std::string GetRenderApiName(hadesmem::cerberus::RenderApi api);

bool& GetGuiVisible() noexcept;

void SetGuiVisible(bool visible, bool old_visible);

void CleanupRender();

void InitializeRender();
}
}
