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

class PluginInterface
{
public:
  virtual ~PluginInterface()
  {
  }

  virtual ModuleInterface* GetModuleInterface() = 0;

  virtual D3D9Interface* GetD3D9Interface() = 0;

  virtual DXGIInterface* GetDXGIInterface() = 0;

  virtual RenderInterface* GetRenderInterface() = 0;
};
}
}
