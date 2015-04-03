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

class GwenInterface;

class HelperInterface;

class ExceptionInterface;

class ProcessInterface;

class OpenGL32Interface;

class D3D10Interface;

class D3D11Interface;

class RawInputInterface;

typedef void OnUnloadPluginsCallback();

class PluginInterface
{
public:
  virtual ~PluginInterface()
  {
  }

  virtual ModuleInterface* GetModuleInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual D3D9Interface* GetD3D9Interface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual DXGIInterface* GetDXGIInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual RenderInterface* GetRenderInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual WindowInterface* GetWindowInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual DirectInputInterface*
    GetDirectInputInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual CursorInterface* GetCursorInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual AntTweakBarInterface*
    GetAntTweakBarInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual GwenInterface* GetGwenInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual HelperInterface* GetHelperInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual ExceptionInterface*
    GetExceptionInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual ProcessInterface* GetProcessInterface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual OpenGL32Interface*
    GetOpenGL32Interface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual D3D10Interface* GetD3D10Interface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual D3D11Interface* GetD3D11Interface() HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual RawInputInterface*
    GetRawInputInterface() HADESMEM_DETAIL_NOEXCEPT = 0;
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
