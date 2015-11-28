// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <pugixml.hpp>
#include <pugixml.cpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
class Config
{
public:
  Config();

  struct Plugin
  {
    std::wstring path_;
    std::wstring process_;
  };

  std::vector<Plugin> GetPlugins() const
  {
    return plugins_;
  }

  bool IsAntTweakBarEnabled() const
  {
    return ant_tweak_bar_enabled_;
  }

  bool IsImguiEnabled() const
  {
    return imgui_enabled_;
  }

  std::vector<std::wstring> GetBlockedProcesses() const
  {
    return blocked_processes_;
  }

private:
  void LoadFile(std::wstring const& path);

  void LoadImpl(pugi::xml_document const& doc);

  std::vector<Plugin> plugins_;
  bool ant_tweak_bar_enabled_{};
  bool imgui_enabled_{};
  std::vector<std::wstring> blocked_processes_;
};

Config& GetConfig();
}
}
