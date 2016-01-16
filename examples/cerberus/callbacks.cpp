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
void LogWrapper(std::string const& s)
{
  auto& imgui = GetImguiInterface();
  imgui.Log(s);
}
}
}
}
