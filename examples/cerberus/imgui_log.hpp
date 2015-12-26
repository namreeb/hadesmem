// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <imgui/imgui.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

namespace hadesmem
{
namespace cerberus
{
class ImGuiLogWindow
{
public:
  void Clear();

  void AddLog(const char* fmt, ...) IM_PRINTFARGS(2);

  void Draw(const char* title, bool* p_opened = NULL);

private:
  ImGuiTextBuffer buf_{};
  ImGuiTextFilter filter_{};
  std::vector<int> line_offsets_{};
  bool scroll_to_bottom_ = false;
};

ImGuiLogWindow& GetImGuiLogWindow();
}
}
