// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "callbacks.hpp"

#include "imgui.hpp"

namespace hadesmem
{
namespace cerberus
{
namespace detail
{
// TODO: Move this somewhere more appropriate. Should probably be abstracted
// away from ImGui and moved to a generic overlay logging layer.
void LogWrapper(std::string const& s)
{
  auto& imgui = GetImguiInterface();
  imgui.Log(s);
}
}
}
}
