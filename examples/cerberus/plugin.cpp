// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "plugin.hpp"

#include <vector>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/trace.hpp>

namespace
{

class D3D9Impl : public hadesmem::cerberus::D3D9Interface
{
public:
  virtual std::size_t RegisterOnFrameCallback(std::function<
    hadesmem::cerberus::OnFrameCallbackD3D9> const& callback) final
  {
    return hadesmem::cerberus::RegisterOnFrameCallbackD3D9(callback);
  }

  virtual void UnregisterOnFrameCallback(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnFrameCallbackD3D9(id);
  }
};

class D3D11Impl : public hadesmem::cerberus::D3D11Interface
{
public:
  virtual std::size_t RegisterOnFrameCallback(std::function<
    hadesmem::cerberus::OnFrameCallbackD3D11> const& callback) final
  {
    return hadesmem::cerberus::RegisterOnFrameCallbackD3D11(callback);
  }

  virtual void UnregisterOnFrameCallback(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnFrameCallbackD3D11(id);
  }
};

class ModuleImpl : public hadesmem::cerberus::ModuleInterface
{
public:
  virtual std::size_t RegisterOnMapCallback(
    std::function<hadesmem::cerberus::OnMapCallback> const& callback) final
  {
    return hadesmem::cerberus::RegisterOnMapCallback(callback);
  }

  virtual void UnregisterOnMapCallback(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnMapCallback(id);
  }

  virtual std::size_t RegisterOnUnmapCallback(
    std::function<hadesmem::cerberus::OnUnmapCallback> const& callback) final
  {
    return hadesmem::cerberus::RegisterOnUnmapCallback(callback);
  }

  virtual void UnregisterOnUnmapCallback(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnUnmapCallback(id);
  }
};

class Plugin : public hadesmem::cerberus::PluginInterface
{
public:
  explicit Plugin(std::wstring const& path) : path_(path)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Loading plugin. [%s]", path.c_str());

    base_ = ::LoadLibraryW(path.c_str());
    if (!base_)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"LoadLibraryW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    HADESMEM_DETAIL_TRACE_FORMAT_A("Loaded plugin. [%p]", base_);

    using LoadFn = void (*)(hadesmem::cerberus::PluginInterface*);
    auto const load_export =
      reinterpret_cast<LoadFn>(::GetProcAddress(base_, "LoadPlugin"));
    if (!load_export)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"GetProcAddress failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
    load_export(this);

    HADESMEM_DETAIL_TRACE_A("Called LoadPlugin export.");
  }

  ~Plugin()
  {
    try
    {
      Unload();
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);
    }
  }

  virtual hadesmem::cerberus::D3D9Interface* GetD3D9Interface() final
  {
    return &d3d9_;
  }

  virtual hadesmem::cerberus::D3D11Interface* GetD3D11Interface() final
  {
    return &d3d11_;
  }

  virtual hadesmem::cerberus::ModuleInterface* GetModuleInterface() final
  {
    return &module_;
  }

  void Unload()
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("Unloading plugin. [%p]", base_);

    using FreeFn = void (*)(hadesmem::cerberus::PluginInterface*);
    auto const unload_export =
      reinterpret_cast<FreeFn>(::GetProcAddress(base_, "UnloadPlugin"));
    if (!unload_export)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"GetProcAddress failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
    unload_export(this);

    HADESMEM_DETAIL_TRACE_A("Called UnloadPlugin export.");

    if (!::FreeLibrary(base_))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"FreeLibrary failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    HADESMEM_DETAIL_TRACE_A("Unloaded plugin.");
  }

private:
  std::wstring path_;
  HMODULE base_{};
  D3D9Impl d3d9_;
  D3D11Impl d3d11_;
  ModuleImpl module_;
};

std::vector<Plugin>& GetPlugins()
{
  static std::vector<Plugin> plugins;
  return plugins;
}
}

namespace hadesmem
{

namespace cerberus
{

void LoadPlugins()
{
  HADESMEM_DETAIL_TRACE_A("Loading plugins.");

  auto& plugins = GetPlugins();
  auto const path = hadesmem::detail::CombinePath(
    hadesmem::detail::GetSelfDirPath(), L"plugin.dll");
  plugins.emplace_back(path);
}

void UnloadPlugins()
{
  HADESMEM_DETAIL_TRACE_A("Unloading plugins.");

  auto& plugins = GetPlugins();
  plugins.clear();
}
}
}
