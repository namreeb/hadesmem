// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <functional>
#include <string>

#include "d3d9.hpp"
#include "dxgi.hpp"
#include "input.hpp"
#include "module.hpp"
#include "render.hpp"

namespace hadesmem
{
namespace cerberus
{
typedef void OnUnloadPluginsCallback();

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

  virtual InputInterface* GetInputInterface() = 0;
};

void LoadPlugins();

void UnloadPlugins();

void LoadPlugin(std::wstring const& path);

void UnloadPlugin(std::wstring const& path);

std::size_t RegisterOnUnloadPlugins(
  std::function<OnUnloadPluginsCallback> const& callback);

void UnregisterOnUnloadPlugins(std::size_t id);
}
}
