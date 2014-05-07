// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "plugin.hpp"

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/trace.hpp>

namespace
{

struct Plugin
{
  std::wstring path;
  HMODULE base;
};

Plugin& GetPlugin()
{
  static Plugin plugin{};
  return plugin;
}

struct CerberusImpl : public CerberusInterface
{
  virtual std::size_t
    RegisterOnFrameCallback(std::function<OnFrameCallback> const& callback)
    final
  {
    return ::RegisterOnFrameCallback(callback);
  }

  virtual void UnregisterOnFrameCallback(std::size_t id) final
  {
    ::UnregisterOnFrameCallback(id);
  }
};

std::unique_ptr<CerberusInterface>& GetPluginInterface()
{
  static std::unique_ptr<CerberusInterface> plugin_interface;
  if (!plugin_interface)
  {
    plugin_interface.reset(new CerberusImpl());
  }
  return plugin_interface;
}
}

void LoadPlugins()
{
  auto& plugin = GetPlugin();
  auto const path = hadesmem::detail::CombinePath(
    hadesmem::detail::GetSelfDirPath(), L"plugin.dll");
  auto const base = ::LoadLibraryW(path.c_str());
  if (!base)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"LoadLibraryW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  plugin = Plugin{path, base};

  using LoadFn = void (*)(CerberusInterface*);
  auto const load_export =
    reinterpret_cast<LoadFn>(::GetProcAddress(plugin.base, "LoadPlugin"));
  if (!load_export)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetProcAddress failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
  load_export(&*GetPluginInterface());
}

void UnloadPlugins()
{
  auto& plugin = GetPlugin();

  using FreeFn = void (*)(CerberusInterface*);
  auto const free_export =
    reinterpret_cast<FreeFn>(::GetProcAddress(plugin.base, "UnloadPlugin"));
  if (!free_export)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetProcAddress failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
  free_export(&*GetPluginInterface());

  if (!::FreeLibrary(plugin.base))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"FreeLibrary failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
}
