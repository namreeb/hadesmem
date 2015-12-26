// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <chaiscript/chaiscript.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

namespace hadesmem
{
namespace cerberus
{
struct ChaiScriptScript
{
public:
  explicit ChaiScriptScript(std::string const& path);

  ChaiScriptScript(ChaiScriptScript const&) = delete;

  ChaiScriptScript& operator=(ChaiScriptScript const&) = delete;

  ChaiScriptScript(ChaiScriptScript&&) = default;

  ChaiScriptScript& operator=(ChaiScriptScript&&) = default;

  ~ChaiScriptScript();

private:
  std::unique_ptr<chaiscript::ChaiScript> chai_;
};

chaiscript::ModulePtr GetCerberusModule();

chaiscript::ChaiScript& GetGlobalChaiScriptContext();

void InitializeChaiScript();
}
}
