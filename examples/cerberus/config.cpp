// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "config.hpp"

#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/pugixml_helpers.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/detail/trace.hpp>

namespace hadesmem
{
namespace cerberus
{
Config& GetConfig()
{
  static Config config;
  return config;
}

Config::Config()
{
  auto const config_path = hadesmem::detail::CombinePath(
    hadesmem::detail::GetSelfDirPath(), L"hadesmem.xml");
  if (hadesmem::detail::DoesFileExist(config_path))
  {
    LoadFile(config_path);
  }
}

void Config::LoadFile(std::wstring const& path)
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

  LoadImpl(doc);
}

// TODO: Fix up the format of the config file so it's all process oriented
// rather than plugin oriented. i.e. You should specify your process, and then
// attach your config for plugins/gui/etc to that (and also have some sort of
// 'global' config which applies for cases where there is no process-specific
// override). That way you can override the GUI on a per-process basis so it
// matches the plugins you're trying to load.
void Config::LoadImpl(pugi::xml_document const& doc)
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

  for (auto const& plugin_node : cerberus_node.children(L"Plugin"))
  {
    auto const path =
      hadesmem::detail::pugixml::GetAttributeValue(plugin_node, L"Path");
    auto const process = hadesmem::detail::pugixml::GetOptionalAttributeValue(
      plugin_node, L"Process");
    plugins_.emplace_back(Plugin{path, process});
  }

  // TODO: Find a better way to do this.
  auto const gui =
    hadesmem::detail::pugixml::GetOptionalAttributeValue(cerberus_node, L"GUI");
  if (hadesmem::detail::ToUpperOrdinal(gui) == L"ANTTWEAKBAR")
  {
    ant_tweak_bar_enabled_ = true;
  }
  else if (hadesmem::detail::ToUpperOrdinal(gui) == L"GWEN")
  {
    gwen_enabled_ = true;
  }
  else
  {
    ant_tweak_bar_enabled_ = false;
  }
}
}
}
