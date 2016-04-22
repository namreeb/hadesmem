// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "plugin.hpp"

#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_helpers.hpp>

#include "ant_tweak_bar.hpp"
#include "callbacks.hpp"
#include "chaiscript.hpp"
#include "config.hpp"
#include "cursor.hpp"
#include "d3d9.hpp"
#include "direct_input.hpp"
#include "dxgi.hpp"
#include "exception.hpp"
#include "helpers.hpp"
#include "imgui.hpp"
#include "module.hpp"
#include "opengl.hpp"
#include "process.hpp"
#include "raw_input.hpp"
#include "render.hpp"
#include "window.hpp"

// TODO: Rethink the design of this. It's sort-of weird for each plugin to get
// its own copy of PluginInterface when all we're doing in the virtual functions
// is returning static data anyway...

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnUnloadPluginsCallback>&
  GetOnUnloadPluginsCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnUnloadPluginsCallback>
    callbacks;
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

      // TODO: This is a hack. Fix it. We should not need to reset contexts on
      // load. Extensions should be able to have their OnInitializeChaiScript
      // callback called to simply extend all existing contexts (or just the
      // default one?).
      HADESMEM_DETAIL_TRACE_A("Resetting default ChaiScript context.");

      hadesmem::cerberus::ReloadDefaultChaiScriptContext(true);
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

  Plugin(Plugin&& other) noexcept : path_(std::move(other.path_)),
                                    base_{other.base_},
                                    unload_{other.unload_},
                                    call_export_{other.call_export_}
  {
    other.base_ = nullptr;
    other.unload_ = false;
    other.call_export_ = false;
  }

  Plugin& operator=(Plugin&& other) noexcept
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
    GetModuleInterface() noexcept final
  {
    return &hadesmem::cerberus::GetModuleInterface();
  }

  virtual hadesmem::cerberus::D3D9Interface* GetD3D9Interface() noexcept final
  {
    return &hadesmem::cerberus::GetD3D9Interface();
  }

  virtual hadesmem::cerberus::DXGIInterface* GetDXGIInterface() noexcept final
  {
    return &hadesmem::cerberus::GetDXGIInterface();
  }

  virtual hadesmem::cerberus::RenderInterface*
    GetRenderInterface() noexcept final
  {
    return &hadesmem::cerberus::GetRenderInterface();
  }

  virtual hadesmem::cerberus::WindowInterface*
    GetWindowInterface() noexcept final
  {
    return &hadesmem::cerberus::GetWindowInterface();
  }

  virtual hadesmem::cerberus::DirectInputInterface*
    GetDirectInputInterface() noexcept final
  {
    return &hadesmem::cerberus::GetDirectInputInterface();
  }

  virtual hadesmem::cerberus::CursorInterface*
    GetCursorInterface() noexcept final
  {
    return &hadesmem::cerberus::GetCursorInterface();
  }

  virtual hadesmem::cerberus::AntTweakBarInterface*
    GetAntTweakBarInterface() noexcept final
  {
    return &hadesmem::cerberus::GetAntTweakBarInterface();
  }

  virtual hadesmem::cerberus::HelperInterface*
    GetHelperInterface() noexcept final
  {
    return &hadesmem::cerberus::GetHelperInterface();
  }

  virtual hadesmem::cerberus::ExceptionInterface*
    GetExceptionInterface() noexcept final
  {
    return &hadesmem::cerberus::GetExceptionInterface();
  }

  virtual hadesmem::cerberus::ProcessInterface*
    GetProcessInterface() noexcept final
  {
    return &hadesmem::cerberus::GetProcessInterface();
  }

  virtual hadesmem::cerberus::OpenGL32Interface*
    GetOpenGL32Interface() noexcept final
  {
    return &hadesmem::cerberus::GetOpenGL32Interface();
  }

  virtual hadesmem::cerberus::RawInputInterface*
    GetRawInputInterface() noexcept final
  {
    return &hadesmem::cerberus::GetRawInputInterface();
  }

  virtual hadesmem::cerberus::ImguiInterface* GetImguiInterface() noexcept final
  {
    return &hadesmem::cerberus::GetImguiInterface();
  }

  virtual hadesmem::cerberus::ChaiScriptInterface*
    GetChaiScriptInterface() noexcept final
  {
    return &hadesmem::cerberus::GetChaiScriptInterface();
  }

  void Unload()
  {
    HADESMEM_DETAIL_TRACE_A("Stopping all ChaiScript scripts.");

    // TODO: Does there need to be synchronization here for callbacks etc
    // running in a different thread? (Also below.)
    auto& scripts = hadesmem::cerberus::GetChaiScriptScripts();
    scripts.clear();

    HADESMEM_DETAIL_TRACE_A("Resetting default ChaiScript context.");

    hadesmem::cerberus::ReloadDefaultChaiScriptContext(false);

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
  void UnloadUnchecked() noexcept
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
    // Hack to ensure the vector doesn't reallocate so plugins can hold on to
    // the pointer to the PluginInterface.
    // TODO: Fix this.
    plugins_.reserve(50);
    GetOnUnloadPluginsCallbacks();
  }

  ~PluginsWrapper()
  {
// TODO: Re-enable this. By disabling this we're leaking on unload/reload.
#if 0
    auto callbacks = GetOnUnloadPluginsCallbacks();
    callbacks.Run();
#endif
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
    std::find_if(std::begin(plugins), std::end(plugins), [&](Plugin const& p) {
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
    std::find_if(std::begin(plugins), std::end(plugins), [&](Plugin const& p) {
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
    HADESMEM_DETAIL_TRACE_FORMAT_W(
      L"Processing config Plugin entry. Path: [%s]. Process: [%s].",
      plugin.path_.c_str(),
      plugin.process_.c_str());

    if (!plugin.process_.empty())
    {
      hadesmem::Process const this_process(::GetCurrentProcessId());
      auto const this_process_path = hadesmem::GetPath(this_process);
      auto const this_process_name =
        this_process_path.substr(this_process_path.rfind(L'\\') + 1);
      if (hadesmem::detail::ToUpperOrdinal(this_process_name) !=
          hadesmem::detail::ToUpperOrdinal(plugin.process_))
      {
        HADESMEM_DETAIL_TRACE_A("Skipping due to process mismatch.");
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

// TODO: Remove the need for this. It's awful and causes just as many problems
// as it solves.
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
