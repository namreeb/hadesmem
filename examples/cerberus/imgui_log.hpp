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
// TODO: Add colours for tags like Info, Warning, Error, etc.
// TODO: Add timestamps to output.
class ImGuiLogWindow
{
public:
  static int const kDefaultBufferSize = 1 * 1024 * 1024;

  ImGuiLogWindow()
  {
    auto const kReserveSize = kDefaultBufferSize + kDefaultBufferSize / 10;
    buf_.Buf.reserve(kReserveSize);
    line_offsets_.reserve(kReserveSize / 20);
  }

  void Clear()
  {
    buf_.clear();
    line_offsets_.clear();
  }

  void AddLog(const char* fmt, ...) IM_PRINTFARGS(2);

  void Draw(char const* title, bool* opened = nullptr);

private:
  ImGuiTextBuffer buf_{};
  ImGuiTextFilter filter_{};
  std::vector<int> line_offsets_{};
  bool scroll_to_bottom_ = false;
};

ImGuiLogWindow& GetImGuiLogWindow();
}
}
