// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <functional>
#include <string>

#include <hadesmem/config.hpp>

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

class AntTweakBarInterface;

class HelperInterface;

class ExceptionInterface;

class ProcessInterface;

class OpenGL32Interface;

class RawInputInterface;

class ImguiInterface;

class ChaiScriptInterface;

typedef void OnUnloadPluginsCallback();

class PluginInterface
{
public:
  virtual ~PluginInterface()
  {
  }

  virtual ModuleInterface* GetModuleInterface() noexcept = 0;

  virtual D3D9Interface* GetD3D9Interface() noexcept = 0;

  virtual DXGIInterface* GetDXGIInterface() noexcept = 0;

  virtual RenderInterface* GetRenderInterface() noexcept = 0;

  virtual WindowInterface* GetWindowInterface() noexcept = 0;

  virtual DirectInputInterface* GetDirectInputInterface() noexcept = 0;

  virtual CursorInterface* GetCursorInterface() noexcept = 0;

  virtual AntTweakBarInterface* GetAntTweakBarInterface() noexcept = 0;

  virtual HelperInterface* GetHelperInterface() noexcept = 0;

  virtual ExceptionInterface* GetExceptionInterface() noexcept = 0;

  virtual ProcessInterface* GetProcessInterface() noexcept = 0;

  virtual OpenGL32Interface* GetOpenGL32Interface() noexcept = 0;

  virtual RawInputInterface* GetRawInputInterface() noexcept = 0;

  virtual ImguiInterface* GetImguiInterface() noexcept = 0;

  virtual ChaiScriptInterface* GetChaiScriptInterface() noexcept = 0;
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
