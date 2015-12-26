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
// Credits to JuJu. https://github.com/JuJuBoSc/imgui-chaiscript
chaiscript::ModulePtr GetImGuiChaiScriptModule();
}
}
