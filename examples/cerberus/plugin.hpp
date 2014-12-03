// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <functional>
#include <string>

namespace hadesmem
{
namespace cerberus
{
class ModuleInterface;

class D3D9Interface;

class DXGIInterface;

class RenderInterface;

class WindowInterface;

class DirectInputInterface;

class CursorInterface;

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

  virtual WindowInterface* GetWindowInterface() = 0;

  virtual DirectInputInterface* GetDirectInputInterface() = 0;

  virtual CursorInterface* GetCursorInterface() = 0;
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
