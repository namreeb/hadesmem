// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "d3d9.hpp"
#include "dxgi.hpp"
#include "module.hpp"
#include "render.hpp"

namespace hadesmem
{

namespace cerberus
{

void LoadPlugins();

void UnloadPlugins();

class D3D9Interface
{
public:
  virtual ~D3D9Interface()
  {
  }

  virtual std::size_t RegisterOnFrameCallback(
    std::function<OnFrameCallbackD3D9> const& callback) = 0;

  virtual void UnregisterOnFrameCallback(std::size_t id) = 0;
};

class DXGIInterface
{
public:
  virtual ~DXGIInterface()
  {
  }

  virtual std::size_t RegisterOnFrameCallback(
    std::function<OnFrameCallbackDXGI> const& callback) = 0;

  virtual void UnregisterOnFrameCallback(std::size_t id) = 0;
};

class ModuleInterface
{
public:
  virtual ~ModuleInterface()
  {
  }

  virtual std::size_t
    RegisterOnMapCallback(std::function<OnMapCallback> const& callback) = 0;

  virtual void UnregisterOnMapCallback(std::size_t id) = 0;

  virtual std::size_t
    RegisterOnUnmapCallback(std::function<OnUnmapCallback> const& callback) = 0;

  virtual void UnregisterOnUnmapCallback(std::size_t id) = 0;
};

class PluginInterface
{
public:
  virtual ~PluginInterface()
  {
  }

  virtual D3D9Interface* GetD3D9Interface() = 0;

  virtual DXGIInterface* GetDXGIInterface() = 0;

  virtual ModuleInterface* GetModuleInterface() = 0;

  virtual RenderInterface* GetRenderInterface() = 0;
};
}
}
