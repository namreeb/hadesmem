// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "plugin.hpp"

#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_helpers.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/detail/trace.hpp>

#include "ant_tweak_bar.hpp"
#include "callbacks.hpp"
#include "config.hpp"
#include "cursor.hpp"
#include "d3d9.hpp"
#include "d3d10.hpp"
#include "d3d11.hpp"
#include "direct_input.hpp"
#include "dxgi.hpp"
#include "exception.hpp"
#include "gwen.hpp"
#include "helpers.hpp"
#include "module.hpp"
#include "opengl.hpp"
#include "process.hpp"
#include "raw_input.hpp"
#include "render.hpp"
#include "window.hpp"

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnUnloadPluginsCallback>&
  GetOnUnloadPluginsCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnUnloadPluginsCallback> callbacks;
  return callbacks;
}

class Plugin : public hadesmem::cerberus::PluginInterface
{
public:
  explicit Plugin(std::wstring const& path) : path_(path)
  {
    try
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
      unload_ = true;

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

      call_export_ = true;
      load_export(this);

      HADESMEM_DETAIL_TRACE_A("Called LoadPlugin export.");
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());

      UnloadUnchecked();

      throw;
    }
  }

  Plugin(Plugin const& other) = delete;

  Plugin& operator=(Plugin const& other) = delete;

  Plugin(Plugin&& other) HADESMEM_DETAIL_NOEXCEPT
    : path_(std::move(other.path_)),
      base_{other.base_},
      unload_{other.unload_},
      call_export_{other.call_export_}
  {
    other.base_ = nullptr;
    other.unload_ = false;
    other.call_export_ = false;
  }

  Plugin& operator=(Plugin&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    UnloadUnchecked();

    path_ = std::move(other.path_);
    base_ = other.base_;
    unload_ = other.unload_;
    call_export_ = other.call_export_;

    other.base_ = nullptr;
    other.unload_ = false;
    other.call_export_ = false;

    return *this;
  }

  ~Plugin()
  {
    UnloadUnchecked();
  }

  virtual hadesmem::cerberus::ModuleInterface*
    GetModuleInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetModuleInterface();
  }

  virtual hadesmem::cerberus::D3D9Interface*
    GetD3D9Interface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetD3D9Interface();
  }

  virtual hadesmem::cerberus::DXGIInterface*
    GetDXGIInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetDXGIInterface();
  }

  virtual hadesmem::cerberus::RenderInterface*
    GetRenderInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetRenderInterface();
  }

  virtual hadesmem::cerberus::WindowInterface*
    GetWindowInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetWindowInterface();
  }

  virtual hadesmem::cerberus::DirectInputInterface*
    GetDirectInputInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetDirectInputInterface();
  }

  virtual hadesmem::cerberus::CursorInterface*
    GetCursorInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetCursorInterface();
  }

  virtual hadesmem::cerberus::AntTweakBarInterface*
    GetAntTweakBarInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetAntTweakBarInterface();
  }

  virtual hadesmem::cerberus::GwenInterface*
    GetGwenInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetGwenInterface();
  }

  virtual hadesmem::cerberus::HelperInterface*
    GetHelperInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetHelperInterface();
  }

  virtual hadesmem::cerberus::ExceptionInterface*
    GetExceptionInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetExceptionInterface();
  }

  virtual hadesmem::cerberus::ProcessInterface*
    GetProcessInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetProcessInterface();
  }

  virtual hadesmem::cerberus::OpenGL32Interface*
    GetOpenGL32Interface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetOpenGL32Interface();
  }

  virtual hadesmem::cerberus::D3D10Interface*
    GetD3D10Interface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetD3D10Interface();
  }

  virtual hadesmem::cerberus::D3D11Interface*
    GetD3D11Interface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetD3D11Interface();
  }

  virtual hadesmem::cerberus::RawInputInterface*
    GetRawInputInterface() HADESMEM_DETAIL_NOEXCEPT final
  {
    return &hadesmem::cerberus::GetRawInputInterface();
  }

  void Unload()
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("Unloading plugin. [%p]", base_);

    if (!unload_)
    {
      HADESMEM_DETAIL_TRACE_A("Nothing to unload.");
      return;
    }

    if (call_export_)
    {
      HADESMEM_DETAIL_TRACE_A("Calling export.");

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
      call_export_ = false;

      HADESMEM_DETAIL_TRACE_A("Called UnloadPlugin export.");
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Not calling export.");
    }

    if (!::FreeLibrary(base_))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"FreeLibrary failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
    unload_ = false;

    HADESMEM_DETAIL_TRACE_A("Unloaded plugin.");
  }

  std::wstring GetPath() const
  {
    return path_;
  }

private:
  void UnloadUnchecked() HADESMEM_DETAIL_NOEXCEPT
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

  std::wstring path_;
  HMODULE base_{};
  bool unload_{};
  bool call_export_{};
};

struct PluginsWrapper
{
  PluginsWrapper()
  {
    GetOnUnloadPluginsCallbacks();
  }

  ~PluginsWrapper()
  {
    auto callbacks = GetOnUnloadPluginsCallbacks();
    callbacks.Run();
  }

  std::vector<Plugin> plugins_;
};

PluginsWrapper& GetPlugins()
{
  static PluginsWrapper plugins;
  return plugins;
}

std::wstring CanonicalizePluginPath(std::wstring path)
{
  if (hadesmem::detail::IsPathRelative(path))
  {
    path =
      hadesmem::detail::CombinePath(hadesmem::detail::GetSelfDirPath(), path);
  }
  path = hadesmem::detail::MakeExtendedPath(path);
  return path;
}
}

namespace hadesmem
{
namespace cerberus
{
void LoadPlugin(std::wstring const& path)
{
  auto& plugins = GetPlugins().plugins_;
  std::wstring path_real = CanonicalizePluginPath(path);
  auto const plugin =
    std::find_if(std::begin(plugins),
                 std::end(plugins),
                 [&](Plugin const& p)
                 {
                   return hadesmem::detail::ToUpperOrdinal(p.GetPath()) ==
                          hadesmem::detail::ToUpperOrdinal(path_real);
                 });
  if (plugin == std::end(plugins))
  {
    plugins.emplace_back(path_real);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(
      L"WARNING! Attempt to reload already loaded plugin. <%s>",
      path_real.c_str());
  }
}

void UnloadPlugin(std::wstring const& path)
{
  auto& plugins = GetPlugins().plugins_;
  std::wstring path_real = CanonicalizePluginPath(path);
  auto const plugin =
    std::find_if(std::begin(plugins),
                 std::end(plugins),
                 [&](Plugin const& p)
                 {
                   return hadesmem::detail::ToUpperOrdinal(p.GetPath()) ==
                          hadesmem::detail::ToUpperOrdinal(path_real);
                 });
  if (plugin == std::end(plugins))
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(
      L"WARNING! Attempt to unload plugin which is not loaded. <%s>",
      path_real.c_str());
  }
  else
  {
    plugins.erase(plugin);
  }
}

void LoadPlugins()
{
  HADESMEM_DETAIL_TRACE_A("Loading plugins.");

  auto const& config = GetConfig();
  for (auto const& plugin : config.GetPlugins())
  {
    if (!plugin.process_.empty())
    {
      hadesmem::Process const this_process(::GetCurrentProcessId());
      auto const this_process_path = hadesmem::GetPath(this_process);
      auto const this_process_name =
        this_process_path.substr(this_process_path.rfind(L'\\') + 1);
      if (hadesmem::detail::ToUpperOrdinal(this_process_name) !=
          hadesmem::detail::ToUpperOrdinal(plugin.process_))
      {
        continue;
      }
    }

    hadesmem::cerberus::LoadPlugin(plugin.path_);
  }
}

void UnloadPlugins()
{
  HADESMEM_DETAIL_TRACE_A("Unloading plugins.");

  auto& plugins = GetPlugins().plugins_;
  plugins.clear();
}

std::size_t RegisterOnUnloadPlugins(
  std::function<hadesmem::cerberus::OnUnloadPluginsCallback> const& callback)
{
  auto& callbacks = GetOnUnloadPluginsCallbacks();
  return callbacks.Register(callback);
}

void UnregisterOnUnloadPlugins(std::size_t id)
{
  auto& callbacks = GetOnUnloadPluginsCallbacks();
  return callbacks.Unregister(id);
}
}
}
