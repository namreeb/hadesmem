// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "imgui_log.hpp"

#include <hadesmem/detail/trace.hpp>

// TODO: Clean this up.

namespace hadesmem
{
namespace cerberus
{
void ImGuiLogWindow::AddLog(const char* fmt, ...) IM_PRINTFARGS(2)
{
  int old_size = buf_.size();

  // Ensure the buffer doesn't get too large.
  // TODO: Fix this properly so we don't lose data unnecessarily. We should
  // remove a line at a time from the top or something similar. A circular
  // buffer may also be useful? What if we also log to file?
  if (old_size > kDefaultBufferSize)
  {
    HADESMEM_DETAIL_TRACE_A("Clearing log.");
    old_size = 0;
    buf_.clear();
    buf_.append("[Warning]: Log cleared.\n");
  }

  va_list args;
  va_start(args, fmt);
  buf_.appendv(fmt, args);
  va_end(args);

  for (int new_size = buf_.size(); old_size < new_size; old_size++)
  {
    if (buf_[old_size] == '\n')
    {
      line_offsets_.push_back(old_size);
    }
  }

  scroll_to_bottom_ = true;
}

void ImGuiLogWindow::Draw(const char* title, bool* p_opened)
{
  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiSetCond_FirstUseEver);
  ImGui::Begin(title, p_opened);

  if (ImGui::Button("Clear"))
  {
    Clear();
  }

  ImGui::SameLine();
  bool copy = ImGui::Button("Copy");

  ImGui::SameLine();
  filter_.Draw("Filter", -80);

  ImGui::Separator();

  ImGui::BeginChild(
    "scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

  if (copy)
  {
    ImGui::LogToClipboard();
  }

  if (filter_.IsActive())
  {
    const char* buf_begin = buf_.begin();
    const char* line = buf_begin;
    for (std::size_t line_no = 0; line != nullptr; ++line_no)
    {
      const char* line_end = (line_no < line_offsets_.size())
                               ? buf_begin + line_offsets_[line_no]
                               : nullptr;
      if (filter_.PassFilter(line, line_end))
      {
        ImGui::TextUnformatted(line, line_end);
      }
      line = line_end && line_end[1] ? line_end + 1 : nullptr;
    }
  }
  else
  {
    ImGui::TextUnformatted(buf_.begin());
  }

  if (scroll_to_bottom_)
  {
    ImGui::SetScrollHere(1.0f);
    scroll_to_bottom_ = false;
  }

  ImGui::EndChild();
  ImGui::End();
}

ImGuiLogWindow& GetImGuiLogWindow()
{
  static ImGuiLogWindow log;
  return log;
}
}
}
