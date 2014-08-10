// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "plugin.hpp"

#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <pugixml.hpp>
#include <pugixml.cpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_helpers.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/pugixml_helpers.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/detail/trace.hpp>

#include "callbacks.hpp"

#if defined(HADESMEM_INTEL)
#pragma warning(push)
#pragma warning(disable : 1345)
#endif // #if defined(HADESMEM_INTEL)

namespace
{

class Plugin : public hadesmem::cerberus::PluginInterface
{
public:
  explicit Plugin(std::wstring const& path) : path_{path}
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
    load_export(this);
    call_export_ = true;

    HADESMEM_DETAIL_TRACE_A("Called LoadPlugin export.");
  }

  Plugin(Plugin const& other) = delete;

  Plugin& operator=(Plugin const& other) = delete;

  Plugin(Plugin&& other) HADESMEM_DETAIL_NOEXCEPT
    : path_{std::move(other.path_)},
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

  virtual hadesmem::cerberus::ModuleInterface* GetModuleInterface() final
  {
    return &hadesmem::cerberus::GetModuleInterface();
  }

  virtual hadesmem::cerberus::D3D9Interface* GetD3D9Interface() final
  {
    return &hadesmem::cerberus::GetD3D9Interface();
  }

  virtual hadesmem::cerberus::DXGIInterface* GetDXGIInterface() final
  {
    return &hadesmem::cerberus::GetDXGIInterface();
  }

  virtual hadesmem::cerberus::RenderInterface* GetRenderInterface() final
  {
    return &hadesmem::cerberus::GetRenderInterface();
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

std::vector<Plugin>& GetPlugins()
{
  static std::vector<Plugin> plugins;
  return plugins;
}

void LoadPluginsFileImpl(pugi::xml_document const& doc)
{
  auto const hadesmem_root = doc.child(L"HadesMem");
  if (!hadesmem_root)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "Failed to find 'HadesMem' root node."});
  }

  auto const cerberus_node = hadesmem_root.child(L"Cerberus");
  if (!cerberus_node)
  {
    return;
  }

  auto& plugins = GetPlugins();
  for (auto const& plugin_node : cerberus_node.children(L"Plugin"))
  {
    auto const process_name =
      hadesmem::detail::pugixml::GetOptionalAttributeValue(plugin_node,
                                                           L"Process");
    if (!process_name.empty())
    {
      hadesmem::Process const this_process(::GetCurrentProcessId());
      auto const this_process_path = hadesmem::GetPath(this_process);
      auto const this_process_name =
        this_process_path.substr(this_process_path.rfind(L'\\') + 1);
      if (hadesmem::detail::ToUpperOrdinal(this_process_name) !=
          hadesmem::detail::ToUpperOrdinal(process_name))
      {
        continue;
      }
    }

    auto path =
      hadesmem::detail::pugixml::GetAttributeValue(plugin_node, L"Path");
    if (hadesmem::detail::IsPathRelative(path))
    {
      path =
        hadesmem::detail::CombinePath(hadesmem::detail::GetSelfDirPath(), path);
    }
    plugins.emplace_back(path);
  }
}

void LoadPluginsFile(std::wstring const& path)
{
  pugi::xml_document doc;
  auto const load_result = doc.load_file(path.c_str());
  if (!load_result)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{}
      << hadesmem::ErrorString{"Loading XML file failed."}
      << hadesmem::ErrorCodeOther{static_cast<DWORD_PTR>(load_result.status)}
      << hadesmem::ErrorStringOther{load_result.description()});
  }

  LoadPluginsFileImpl(doc);
}

void LoadPluginsMemory(std::wstring const& data)
{
  pugi::xml_document doc;
  auto const load_result = doc.load(data.c_str());
  if (!load_result)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{}
      << hadesmem::ErrorString{"Loading XML file failed."}
      << hadesmem::ErrorCodeOther{static_cast<DWORD_PTR>(load_result.status)}
      << hadesmem::ErrorStringOther{load_result.description()});
  }

  LoadPluginsFileImpl(doc);
}
}

namespace hadesmem
{

namespace cerberus
{

void LoadPlugins()
{
  HADESMEM_DETAIL_TRACE_A("Loading plugins.");

  auto const config_path = hadesmem::detail::CombinePath(
    hadesmem::detail::GetSelfDirPath(), L"hadesmem.xml");
  if (hadesmem::detail::DoesFileExist(config_path))
  {
    LoadPluginsFile(config_path);
  }
  else
  {
#if defined(HADESMEM_GCC)
    std::wstring const config_xml = LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <Cerberus>
    <Plugin Path="libplugin.dll"/>
  </Cerberus>
</HadesMem>
)";
#else
    std::wstring const config_xml = LR"(
<?xml version="1.0" encoding="utf-8"?>
<HadesMem>
  <Cerberus>
    <Plugin Path="plugin.dll"/>
  </Cerberus>
</HadesMem>
)";
#endif
    LoadPluginsMemory(config_xml);
  }
}

void UnloadPlugins()
{
  HADESMEM_DETAIL_TRACE_A("Unloading plugins.");

  auto& plugins = GetPlugins();
  plugins.clear();
}
}
}

#if defined(HADESMEM_INTEL)
#pragma warning(pop)
#endif // #if defined(HADESMEM_INTEL)
