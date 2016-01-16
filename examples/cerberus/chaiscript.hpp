// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <chaiscript/chaiscript.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
typedef void
  OnInitializeChaiScriptContextCallback(chaiscript::ChaiScript& chai);

class ChaiScriptInterface
{
public:
  virtual ~ChaiScriptInterface()
  {
  }

  virtual std::size_t RegisterOnInitializeChaiScriptContext(
    std::function<OnInitializeChaiScriptContextCallback> const& callback) = 0;

  virtual void UnregisterOnInitializeChaiScriptContext(std::size_t id) = 0;

  virtual chaiscript::ChaiScript& GetGlobalContext() = 0;
};

class ChaiScriptScript
{
public:
  explicit ChaiScriptScript(std::string const& path);

  ChaiScriptScript(ChaiScriptScript const&) = delete;

  ChaiScriptScript& operator=(ChaiScriptScript const&) = delete;

  ChaiScriptScript(ChaiScriptScript&&) = default;

  ChaiScriptScript& operator=(ChaiScriptScript&&) = default;

  ~ChaiScriptScript();

private:
  std::string path_;
  std::unique_ptr<chaiscript::ChaiScript> chai_;
};

std::map<std::string, ChaiScriptScript>& GetChaiScriptScripts();

chaiscript::ChaiScript& GetGlobalChaiScriptContext();

ChaiScriptInterface& GetChaiScriptInterface() noexcept;

void ReloadDefaultChaiScriptContext(bool run_callbacks);
}
}
