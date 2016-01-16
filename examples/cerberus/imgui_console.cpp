// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "imgui_console.hpp"

#include <cctype>
#include <functional>
#include <cstdlib>

#include <hadesmem/error.hpp>

#include "chaiscript.hpp"

// TODO: Clean this up.

namespace hadesmem
{
namespace cerberus
{
ImGuiConsoleWindow::ImGuiConsoleWindow()
{
}

ImGuiConsoleWindow::~ImGuiConsoleWindow()
{
  ClearLog();
}

void ImGuiConsoleWindow::ClearLog()
{
  items_.clear();
  scroll_to_bottom_ = true;
}

void ImGuiConsoleWindow::AddLog(const char* fmt, ...) IM_PRINTFARGS(2)
{
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, _countof(buf), fmt, args);
  buf[_countof(buf) - 1] = 0;
  va_end(args);
  items_.push_back(_strdup(buf));
  scroll_to_bottom_ = true;
}

void ImGuiConsoleWindow::Draw(const char* title, bool* opened)
{
  ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiSetCond_FirstUseEver);
  if (!ImGui::Begin(title, opened))
  {
    ImGui::End();
    return;
  }

  // TODO: Display items starting from the bottom.

  if (ImGui::Button("Clear"))
  {
    ClearLog();
  }
  ImGui::SameLine();

  bool copy = ImGui::Button("Copy");
  ImGui::SameLine();

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
  static ImGuiTextFilter filter;
  filter.Draw("Filter", -80);
  ImGui::PopStyleVar();

  ImGui::Separator();

  // TODO: Read the comments below and ensure perf is adequate (and fix if
  // not).

  // Display every line as a separate entry so we can change their color or
  // add custom widgets. If you only want raw text you can use
  // ImGui::TextUnformatted(log.begin(), log.end());
  // NB- if you have thousands of entries this approach may be too
  // inefficient. You can seek and display only the lines that are visible -
  // CalcListClipping() is a helper to compute this information.
  // If your items are of variable size you may want to implement code similar
  // to what CalcListClipping() does. Or split your data into fixed height
  // items to allow random-seeking into your list.
  ImGui::BeginChild("ScrollingRegion",
                    ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()),
                    false,
                    ImGuiWindowFlags_HorizontalScrollbar);

  if (copy)
  {
    ImGui::LogToClipboard();
  }

  // Tighten spacing
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
  for (std::size_t i = 0; i < items_.size(); i++)
  {
    auto const item = items_[i].c_str();
    if (!filter.PassFilter(item))
    {
      continue;
    }

    // A better implementation may store a type per-item. For the sample let's
    // just parse the text.
    ImVec4 col = ImColor(255, 255, 255);
    if (std::strstr(item, "[Error]"))
    {
      col = ImColor(255, 100, 100);
    }
    else if (std::strncmp(item, "# ", 2) == 0)
    {
      col = ImColor(255, 200, 150);
    }

    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::TextUnformatted(item);
    ImGui::PopStyleColor();
  }

  if (scroll_to_bottom_)
  {
    ImGui::SetScrollHere();
  }

  scroll_to_bottom_ = false;
  ImGui::PopStyleVar();
  ImGui::EndChild();

  if (copy)
  {
    ImGui::LogFinish();
  }

  ImGui::Separator();

  auto const submit_command = [&]()
  {

    char* input_end = input_buf_ + std::strlen(input_buf_);
    while (input_end > input_buf_ && input_end[-1] == ' ')
    {
      input_end--;
    }
    *input_end = 0;

    if (input_buf_[0])
    {
      ExecCommand(input_buf_);
    }

    std::strcpy(input_buf_, "");
  };

  auto const text_edit_callback_stub = [](ImGuiTextEditCallbackData* data)
  {
    ImGuiConsoleWindow* console =
      static_cast<ImGuiConsoleWindow*>(data->UserData);
    return console->TextEditCallback(data);
  };

  ImGui::PushItemWidth(-80);
  if (ImGui::InputText("",
                       input_buf_,
                       _countof(input_buf_),
                       ImGuiInputTextFlags_EnterReturnsTrue |
                         ImGuiInputTextFlags_CallbackCompletion |
                         ImGuiInputTextFlags_CallbackHistory,
                       text_edit_callback_stub,
                       (void*)this))
  {
    submit_command();
  }
  ImGui::PopItemWidth();

  // Auto focus
  if (ImGui::IsItemHovered() ||
      (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() &&
       !ImGui::IsMouseClicked(0)))
  {
    ImGui::SetKeyboardFocusHere(-1);
  }

  ImGui::SameLine();
  if (ImGui::Button("Submit"))
  {
    submit_command();
  }

  ImGui::End();
}

int ImGuiConsoleWindow::Stricmp(const char* str1, const char* str2)
{
  int d;
  while ((d = std::toupper(*str2) - std::toupper(*str1)) == 0 && *str1)
  {
    str1++;
    str2++;
  }
  return d;
}

int ImGuiConsoleWindow::Strnicmp(const char* str1, const char* str2, int count)
{
  int d = 0;
  while (count > 0 && (d = std::toupper(*str2) - std::toupper(*str1)) == 0 &&
         *str1)
  {
    str1++;
    str2++;
    count--;
  }
  return d;
}

void ImGuiConsoleWindow::ExecCommand(const char* command_line)
{
  AddLog("# %s\n", command_line);

  // Insert into history. First find match and delete it so it can be pushed
  // to the back. This isn't trying to be smart or optimal.
  // TODO: Use a smarter algorithm.
  history_pos_ = -1;
  for (std::int32_t i = static_cast<std::int32_t>(history_.size()) - 1; i >= 0;
       i--)
  {
    if (Stricmp(history_[i].c_str(), command_line) == 0)
    {
      history_.erase(history_.begin() + i);
      break;
    }
  }
  history_.push_back(_strdup(command_line));

  if (Stricmp(command_line, "/CLEAR") == 0)
  {
    ClearLog();
  }
  else if (Stricmp(command_line, "/HELP") == 0)
  {
    AddLog("Commands:");
    for (std::size_t i = 0; i < commands_.size(); i++)
    {
      AddLog("- %s", commands_[i].c_str());
    }
  }
  else if (Stricmp(command_line, "/HISTORY") == 0)
  {
    for (std::size_t i = history_.size() >= 10 ? history_.size() - 10 : 0;
         i < history_.size();
         i++)
    {
      AddLog("%3d: %s\n", i, history_[i].c_str());
    }
  }
  else if (Stricmp(command_line, "/TERMINATE") == 0)
  {
    std::terminate();
  }
  else
  {
    try
    {
      auto& chai = GetGlobalChaiScriptContext();
      auto const val = chai.eval(command_line);
      if (!val.get_type_info().bare_equal(chaiscript::user_type<void>()))
      {
        try
        {
          auto const str = chai.eval<
            std::function<std::string(const chaiscript::Boxed_Value& bv)>>(
            "to_string")(val);
          AddLog("%s\n", str.c_str());
        }
        catch (...)
        {
        }
      }
    }
    catch (const chaiscript::exception::eval_error& ee)
    {
      if (ee.call_stack.size() > 0)
      {
        AddLog("[Error]: %s during evaluation at (%d,%d)\n",
               boost::current_exception_diagnostic_information().c_str(),
               ee.call_stack[0]->start().line,
               ee.call_stack[0]->start().column);
      }
      else
      {
        AddLog("[Error]: %s",
               boost::current_exception_diagnostic_information().c_str());
      }
    }
    catch (...)
    {
      AddLog("[Error]: %s",
             boost::current_exception_diagnostic_information().c_str());
    }
  }
}

int ImGuiConsoleWindow::TextEditCallback(ImGuiTextEditCallbackData* data)
{
  switch (data->EventFlag)
  {
  case ImGuiInputTextFlags_CallbackCompletion:
  {
    const char* word_end = data->Buf + data->CursorPos;
    const char* word_start = word_end;
    while (word_start > data->Buf)
    {
      const char c = word_start[-1];
      if (c == ' ' || c == '\t' || c == ',' || c == ';')
        break;
      word_start--;
    }

    ImVector<const char*> candidates;
    for (std::size_t i = 0; i < commands_.size(); i++)
    {
      if (Strnicmp(commands_[i].c_str(),
                   word_start,
                   (int)(word_end - word_start)) == 0)
      {
        candidates.push_back(commands_[i].c_str());
      }
    }

    if (candidates.Size == 0)
    {
      AddLog(
        "No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
    }
    else if (candidates.Size == 1)
    {
      // Single match. Delete the beginning of the word and replace it
      // entirely so we've got nice casing
      data->DeleteChars((int)(word_start - data->Buf),
                        (int)(word_end - word_start));
      data->InsertChars(data->CursorPos, candidates[0]);
      data->InsertChars(data->CursorPos, " ");
    }
    else
    {
      // Multiple matches. Complete as much as we can, so inputing "C" will
      // complete to "CL" and display "CLEAR" and "CLASSIFY"
      int match_len = (int)(word_end - word_start);
      for (;;)
      {
        int c = 0;
        bool all_candidates_matches = true;
        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
          if (i == 0)
            c = toupper(candidates[i][match_len]);
          else if (c != toupper(candidates[i][match_len]))
            all_candidates_matches = false;
        if (!all_candidates_matches)
          break;
        match_len++;
      }

      if (match_len > 0)
      {
        data->DeleteChars((int)(word_start - data->Buf),
                          (int)(word_end - word_start));
        data->InsertChars(
          data->CursorPos, candidates[0], candidates[0] + match_len);
      }

      // List matches
      AddLog("Possible matches:\n");
      for (int i = 0; i < candidates.Size; i++)
      {
        AddLog("- %s\n", candidates[i]);
      }
    }

    break;
  }
  case ImGuiInputTextFlags_CallbackHistory:
  {
    const int prev_history_pos = history_pos_;
    if (data->EventKey == ImGuiKey_UpArrow)
    {
      if (history_pos_ == -1)
      {
        history_pos_ = static_cast<int>(history_.size()) - 1;
      }
      else if (history_pos_ > 0)
      {
        history_pos_--;
      }
    }
    else if (data->EventKey == ImGuiKey_DownArrow)
    {
      if (history_pos_ != -1 &&
          ++history_pos_ >= static_cast<int>(history_.size()))
      {
        history_pos_ = -1;
      }
    }

    // TODO: A better implementation would preserve the data on the current
    // input line along with cursor position.
    if (prev_history_pos != history_pos_)
    {
      std::snprintf(data->Buf,
                    data->BufSize,
                    "%s",
                    (history_pos_ >= 0) ? history_[history_pos_].c_str() : "");
      data->BufDirty = true;
      data->CursorPos = data->SelectionStart = data->SelectionEnd =
        static_cast<int>(std::strlen(data->Buf));
    }
  }
  }

  return 0;
}

ImGuiConsoleWindow& GetImGuiConsoleWindow()
{
  static ImGuiConsoleWindow console;
  return console;
}
}
}
