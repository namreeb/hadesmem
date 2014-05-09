// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "d3d11.hpp"
#include "module.hpp"

namespace hadesmem
{

namespace cerberus
{

void LoadPlugins();

void UnloadPlugins();

class D3D11Interface
{
public:
  virtual ~D3D11Interface()
  {
  }

  virtual std::size_t
    RegisterOnFrameCallback(std::function<OnFrameCallback> const& callback) = 0;

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

  virtual D3D11Interface* GetD3D11Interface() = 0;

  virtual ModuleInterface* GetModuleInterface() = 0;
};
}
}
