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
class ImGuiConsoleWindow
{
public:
  ImGuiConsoleWindow();

  ~ImGuiConsoleWindow();

  void ClearLog();

  void AddLog(const char* fmt, ...) IM_PRINTFARGS(2);

  void Draw(const char* title, bool* opened);

  static int Stricmp(const char* str1, const char* str2);

  static int Strnicmp(const char* str1, const char* str2, int count);

  void ExecCommand(const char* command_line);

  int TextEditCallback(ImGuiTextEditCallbackData* data);

private:
  char input_buf_[256] = {};
  std::vector<std::string> items_;
  bool scroll_to_bottom_ = true;
  std::vector<std::string> history_;
  // -1: new line, 0..history_.size()-1 browsing history.
  int history_pos_ = -1;
  // TODO: Don't duplicate command list between here and in the exec func.
  std::vector<std::string> commands_{
    "/HELP", "/HISTORY", "/CLEAR", "/TERMINATE"};
};

ImGuiConsoleWindow& GetImGuiConsoleWindow();
}
}
