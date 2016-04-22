// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "imgui.hpp"

#include <windows.h>

#include <d3d11.h>
#include <d3d10.h>
#include <d3d9.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <imgui/imgui.h>
#include <imgui/examples/directx9_example/imgui_impl_dx9.h>
#include <imgui/examples/directx10_example/imgui_impl_dx10.h>
#include <imgui/examples/directx11_example/imgui_impl_dx11.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/dump.hpp>
#include <hadesmem/detail/filesystem.hpp>

#include "callbacks.hpp"
#include "chaiscript.hpp"
#include "cursor.hpp"
#include "hook_disabler.hpp"
#include "imgui_console.hpp"
#include "imgui_log.hpp"
#include "input.hpp"
#include "main.hpp"
#include "plugin.hpp"
#include "render.hpp"
#include "window.hpp"

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd,
                                             UINT msg,
                                             WPARAM wParam,
                                             LPARAM lParam);

// TODO: Fix thread safety of initialization etc.

// TODO: Consolidate code across different GUIs where possible.

// TODO: Consolidate all notes/todos across different GUIs where they are not
// GUI-specific.

// TODO: Support multiple contexts by using ImGui::SetInternalState.
// See here: https://github.com/ocornut/imgui/issues/207
// See here: https://github.com/ocornut/imgui/issues/269

// TODO: Stop dropping imgui.ini files in game directories.

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnImguiInitializeCallback>&
  GetOnImguiInitializeCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnImguiInitializeCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnImguiCleanupCallback>&
  GetOnImguiCleanupCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnImguiCleanupCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnImguiFrameCallback>&
  GetOnImguiFrameCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnImguiFrameCallback>
    callbacks;
  return callbacks;
}

bool& GetImguiInitialized(hadesmem::cerberus::RenderApi api)
{
  if (api == hadesmem::cerberus::RenderApi::kD3D9)
  {
    static bool initialized{false};
    return initialized;
  }
  else if (api == hadesmem::cerberus::RenderApi::kD3D10)
  {
    static bool initialized{false};
    return initialized;
  }
  else if (api == hadesmem::cerberus::RenderApi::kD3D11)
  {
    static bool initialized{false};
    return initialized;
  }
  else if (api == hadesmem::cerberus::RenderApi::kOpenGL32)
  {
    static bool initialized{false};
    return initialized;
  }
  else
  {
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
  }
}

void SetImguiInitialized(hadesmem::cerberus::RenderApi api, bool value)
{
  auto& initialized = GetImguiInitialized(api);
  initialized = value;
}

bool ImguiInitializedAny() noexcept
{
  return GetImguiInitialized(hadesmem::cerberus::RenderApi::kD3D9) ||
         GetImguiInitialized(hadesmem::cerberus::RenderApi::kD3D10) ||
         GetImguiInitialized(hadesmem::cerberus::RenderApi::kD3D11) ||
         GetImguiInitialized(hadesmem::cerberus::RenderApi::kOpenGL32);
}

class ImguiImpl : public hadesmem::cerberus::ImguiInterface
{
public:
  ~ImguiImpl()
  {
    SetImguiInitialized(hadesmem::cerberus::RenderApi::kD3D9, false);
    SetImguiInitialized(hadesmem::cerberus::RenderApi::kD3D10, false);
    SetImguiInitialized(hadesmem::cerberus::RenderApi::kD3D11, false);
    SetImguiInitialized(hadesmem::cerberus::RenderApi::kOpenGL32, false);
  }

  virtual std::size_t RegisterOnInitialize(
    std::function<hadesmem::cerberus::OnImguiInitializeCallback> const&
      callback) final
  {
    auto& callbacks = GetOnImguiInitializeCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnInitialize(std::size_t id) final
  {
    auto& callbacks = GetOnImguiInitializeCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnCleanup(
    std::function<hadesmem::cerberus::OnImguiCleanupCallback> const& callback)
    final
  {
    auto& callbacks = GetOnImguiCleanupCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnCleanup(std::size_t id) final
  {
    auto& callbacks = GetOnImguiCleanupCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnFrame(
    std::function<hadesmem::cerberus::OnImguiFrameCallback> const& callback)
    final
  {
    auto& callbacks = GetOnImguiFrameCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnFrame(std::size_t id) final
  {
    auto& callbacks = GetOnImguiFrameCallbacks();
    return callbacks.Unregister(id);
  }

  virtual void Log(std::string const& s) final
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("%s", s.c_str());

    auto& log = hadesmem::cerberus::GetImGuiLogWindow();
    log.AddLog("%s\n", s.c_str());
  }

  virtual bool IsInitialized() final
  {
    return ImguiInitializedAny();
  }

  // Main
  virtual ImGuiIO& GetIO() final
  {
    return ImGui::GetIO();
  }
  virtual ImGuiStyle& GetStyle() final
  {
    return ImGui::GetStyle();
  }
  virtual ImDrawData* GetDrawData() final
  {
    return ImGui::GetDrawData();
  }
  virtual void NewFrame() final
  {
    return ImGui::NewFrame();
  }
  virtual void Render() final
  {
    return ImGui::Render();
  }
  virtual void Shutdown() final
  {
    return ImGui::Shutdown();
  }
  virtual void ShowUserGuide() final
  {
    return ImGui::ShowUserGuide();
  }
  virtual void ShowStyleEditor(ImGuiStyle* ref = NULL) final
  {
    return ImGui::ShowStyleEditor(ref);
  }
  virtual void ShowTestWindow(bool* opened = NULL) final
  {
    return ImGui::ShowTestWindow(opened);
  }
  virtual void ShowMetricsWindow(bool* opened = NULL) final
  {
    return ImGui::ShowMetricsWindow(opened);
  }

  // Window
  virtual bool Begin(const char* name,
                     bool* p_opened = NULL,
                     ImGuiWindowFlags flags = 0) final
  {
    return ImGui::Begin(name, p_opened, flags);
  }
  virtual bool Begin(const char* name,
                     bool* p_opened,
                     const ImVec2& size_on_first_use,
                     float bg_alpha = -1.0f,
                     ImGuiWindowFlags flags = 0) final
  {
    return ImGui::Begin(name, p_opened, size_on_first_use, bg_alpha, flags);
  }
  virtual void End() final
  {
    return ImGui::End();
  }
  virtual bool BeginChild(const char* str_id,
                          const ImVec2& size = ImVec2(0, 0),
                          bool border = false,
                          ImGuiWindowFlags extra_flags = 0) final
  {
    return ImGui::BeginChild(str_id, size, border, extra_flags);
  }
  virtual bool BeginChild(ImGuiID id,
                          const ImVec2& size = ImVec2(0, 0),
                          bool border = false,
                          ImGuiWindowFlags extra_flags = 0) final
  {
    return ImGui::BeginChild(id, size, border, extra_flags);
  }
  virtual void EndChild() final
  {
    return ImGui::EndChild();
  }
  virtual ImVec2 GetContentRegionMax() final
  {
    return ImGui::GetContentRegionMax();
  }
  virtual ImVec2 GetContentRegionAvail() final
  {
    return ImGui::GetContentRegionAvail();
  }
  virtual float GetContentRegionAvailWidth() final
  {
    return ImGui::GetContentRegionAvailWidth();
  }
  virtual ImVec2 GetWindowContentRegionMin() final
  {
    return ImGui::GetWindowContentRegionMin();
  }
  virtual ImVec2 GetWindowContentRegionMax() final
  {
    return ImGui::GetWindowContentRegionMax();
  }
  virtual float GetWindowContentRegionWidth() final
  {
    return ImGui::GetWindowContentRegionWidth();
  }
  virtual ImDrawList* GetWindowDrawList() final
  {
    return ImGui::GetWindowDrawList();
  }
  virtual ImFont* GetWindowFont() final
  {
    return ImGui::GetWindowFont();
  }
  virtual float GetWindowFontSize() final
  {
    return ImGui::GetWindowFontSize();
  }
  virtual void SetWindowFontScale(float scale) final
  {
    return ImGui::SetWindowFontScale(scale);
  }
  virtual ImVec2 GetWindowPos() final
  {
    return ImGui::GetWindowPos();
  }
  virtual ImVec2 GetWindowSize() final
  {
    return ImGui::GetWindowSize();
  }
  virtual float GetWindowWidth() final
  {
    return ImGui::GetWindowWidth();
  }
  virtual float GetWindowHeight() final
  {
    return ImGui::GetWindowHeight();
  }
  virtual bool IsWindowCollapsed() final
  {
    return ImGui::IsWindowCollapsed();
  }

  virtual void SetNextWindowPos(const ImVec2& pos, ImGuiSetCond cond = 0) final
  {
    return ImGui::SetNextWindowPos(pos, cond);
  }
  virtual void SetNextWindowPosCenter(ImGuiSetCond cond = 0) final
  {
    return ImGui::SetNextWindowPosCenter(cond);
  }
  virtual void SetNextWindowSize(const ImVec2& size,
                                 ImGuiSetCond cond = 0) final
  {
    return ImGui::SetNextWindowSize(size, cond);
  }
  virtual void SetNextWindowContentSize(const ImVec2& size) final
  {
    return ImGui::SetNextWindowContentSize(size);
  }
  virtual void SetNextWindowContentWidth(float width) final
  {
    return ImGui::SetNextWindowContentWidth(width);
  }
  virtual void SetNextWindowCollapsed(bool collapsed,
                                      ImGuiSetCond cond = 0) final
  {
    return ImGui::SetNextWindowCollapsed(collapsed, cond);
  }
  virtual void SetNextWindowFocus() final
  {
    return ImGui::SetNextWindowFocus();
  }
  virtual void SetWindowPos(const ImVec2& pos, ImGuiSetCond cond = 0) final
  {
    return ImGui::SetWindowPos(pos, cond);
  }
  virtual void SetWindowSize(const ImVec2& size, ImGuiSetCond cond = 0) final
  {
    return ImGui::SetWindowSize(size, cond);
  }
  virtual void SetWindowCollapsed(bool collapsed, ImGuiSetCond cond = 0) final
  {
    return ImGui::SetWindowCollapsed(collapsed, cond);
  }
  virtual void SetWindowFocus() final
  {
    return ImGui::SetWindowFocus();
  }
  virtual void SetWindowPos(const char* name,
                            const ImVec2& pos,
                            ImGuiSetCond cond = 0) final
  {
    return ImGui::SetWindowPos(name, pos, cond);
  }
  virtual void SetWindowSize(const char* name,
                             const ImVec2& size,
                             ImGuiSetCond cond = 0) final
  {
    return ImGui::SetWindowSize(name, size, cond);
  }
  virtual void SetWindowCollapsed(const char* name,
                                  bool collapsed,
                                  ImGuiSetCond cond = 0) final
  {
    return ImGui::SetWindowCollapsed(name, collapsed, cond);
  }
  virtual void SetWindowFocus(const char* name) final
  {
    return ImGui::SetWindowFocus(name);
  }

  virtual float GetScrollX() final
  {
    return ImGui::GetScrollX();
  }
  virtual float GetScrollY() final
  {
    return ImGui::GetScrollY();
  }
  virtual float GetScrollMaxX() final
  {
    return ImGui::GetScrollMaxX();
  }
  virtual float GetScrollMaxY() final
  {
    return ImGui::GetScrollMaxY();
  }
  virtual void SetScrollX(float scroll_x) final
  {
    return ImGui::SetScrollX(scroll_x);
  }
  virtual void SetScrollY(float scroll_y) final
  {
    return ImGui::SetScrollY(scroll_y);
  }
  virtual void SetScrollHere(float center_y_ratio = 0.5f) final
  {
    return ImGui::SetScrollHere(center_y_ratio);
  }
  virtual void SetScrollFromPosY(float pos_y, float center_y_ratio = 0.5f) final
  {
    return ImGui::SetScrollFromPosY(pos_y, center_y_ratio);
  }
  virtual void SetKeyboardFocusHere(int offset = 0) final
  {
    return ImGui::SetKeyboardFocusHere(offset);
  }
  virtual void SetStateStorage(ImGuiStorage* tree) final
  {
    return ImGui::SetStateStorage(tree);
  }
  virtual ImGuiStorage* GetStateStorage() final
  {
    return ImGui::GetStateStorage();
  }

  // Parameters stacks (shared)
  virtual void PushFont(ImFont* font) final
  {
    return ImGui::PushFont(font);
  }
  virtual void PopFont() final
  {
    return ImGui::PopFont();
  }
  virtual void PushStyleColor(ImGuiCol idx, const ImVec4& col) final
  {
    return ImGui::PushStyleColor(idx, col);
  }
  virtual void PopStyleColor(int count = 1) final
  {
    return ImGui::PopStyleColor(count);
  }
  virtual void PushStyleVar(ImGuiStyleVar idx, float val) final
  {
    return ImGui::PushStyleVar(idx, val);
  }
  virtual void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val) final
  {
    return ImGui::PushStyleVar(idx, val);
  }
  virtual void PopStyleVar(int count = 1) final
  {
    return ImGui::PopStyleVar(count);
  }

  // Parameters stacks (current window)
  virtual void PushItemWidth(float item_width) final
  {
    return ImGui::PushItemWidth(item_width);
  }
  virtual void PopItemWidth() final
  {
    return ImGui::PopItemWidth();
  }
  virtual float CalcItemWidth() final
  {
    return ImGui::CalcItemWidth();
  }
  virtual void PushTextWrapPos(float wrap_pos_x = 0.0f) final
  {
    return ImGui::PushTextWrapPos(wrap_pos_x);
  }
  virtual void PopTextWrapPos() final
  {
    return ImGui::PopTextWrapPos();
  }
  virtual void PushAllowKeyboardFocus(bool v) final
  {
    return ImGui::PushAllowKeyboardFocus(v);
  }
  virtual void PopAllowKeyboardFocus() final
  {
    return ImGui::PopAllowKeyboardFocus();
  }
  virtual void PushButtonRepeat(bool repeat) final
  {
    return ImGui::PushButtonRepeat(repeat);
  }
  virtual void PopButtonRepeat() final
  {
    return ImGui::PopButtonRepeat();
  }

  // Cursor / Layout
  virtual void BeginGroup() final
  {
    return ImGui::BeginGroup();
  }
  virtual void EndGroup() final
  {
    return ImGui::EndGroup();
  }
  virtual void Separator() final
  {
    return ImGui::Separator();
  }
  virtual void SameLine(float local_pos_x = 0.0f, float spacing_w = -1.0f) final
  {
    return ImGui::SameLine(local_pos_x, spacing_w);
  }
  virtual void Spacing() final
  {
    return ImGui::Spacing();
  }
  virtual void Dummy(const ImVec2& size) final
  {
    return ImGui::Dummy(size);
  }
  virtual void Indent() final
  {
    return ImGui::Indent();
  }
  virtual void Unindent() final
  {
    return ImGui::Unindent();
  }
  virtual void
    Columns(int count = 1, const char* id = NULL, bool border = true) final
  {
    return ImGui::Columns(count, id, border);
  }
  virtual void NextColumn() final
  {
    return ImGui::NextColumn();
  }
  virtual int GetColumnIndex() final
  {
    return ImGui::GetColumnIndex();
  }
  virtual float GetColumnOffset(int column_index = -1) final
  {
    return ImGui::GetColumnOffset(column_index);
  }
  virtual void SetColumnOffset(int column_index, float offset_x) final
  {
    return ImGui::SetColumnOffset(column_index, offset_x);
  }
  virtual float GetColumnWidth(int column_index = -1) final
  {
    return ImGui::GetColumnWidth(column_index);
  }
  virtual int GetColumnsCount() final
  {
    return ImGui::GetColumnsCount();
  }
  virtual ImVec2 GetCursorPos() final
  {
    return ImGui::GetCursorPos();
  }
  virtual float GetCursorPosX() final
  {
    return ImGui::GetCursorPosX();
  }
  virtual float GetCursorPosY() final
  {
    return ImGui::GetCursorPosY();
  }
  virtual void SetCursorPos(const ImVec2& local_pos) final
  {
    return ImGui::SetCursorPos(local_pos);
  }
  virtual void SetCursorPosX(float x) final
  {
    return ImGui::SetCursorPosX(x);
  }
  virtual void SetCursorPosY(float y) final
  {
    return ImGui::SetCursorPosY(y);
  }
  virtual ImVec2 GetCursorStartPos() final
  {
    return ImGui::GetCursorStartPos();
  }
  virtual ImVec2 GetCursorScreenPos() final
  {
    return ImGui::GetCursorScreenPos();
  }
  virtual void SetCursorScreenPos(const ImVec2& pos) final
  {
    return ImGui::SetCursorScreenPos(pos);
  }
  virtual void AlignFirstTextHeightToWidgets() final
  {
    return ImGui::AlignFirstTextHeightToWidgets();
  }
  virtual float GetTextLineHeight() final
  {
    return ImGui::GetTextLineHeight();
  }
  virtual float GetTextLineHeightWithSpacing() final
  {
    return ImGui::GetTextLineHeightWithSpacing();
  }
  virtual float GetItemsLineHeightWithSpacing() final
  {
    return ImGui::GetItemsLineHeightWithSpacing();
  }

  // ID scopes
  virtual void PushID(const char* str_id) final
  {
    return ImGui::PushID(str_id);
  }
  virtual void PushID(const char* str_id_begin, const char* str_id_end) final
  {
    return ImGui::PushID(str_id_begin, str_id_end);
  }
  virtual void PushID(const void* ptr_id) final
  {
    return ImGui::PushID(ptr_id);
  }
  virtual void PushID(int int_id) final
  {
    return ImGui::PushID(int_id);
  }
  virtual void PopID() final
  {
    return ImGui::PopID();
  }
  virtual ImGuiID GetID(const char* str_id) final
  {
    return ImGui::GetID(str_id);
  }
  virtual ImGuiID GetID(const char* str_id_begin, const char* str_id_end) final
  {
    return ImGui::GetID(str_id_begin, str_id_end);
  }
  virtual ImGuiID GetID(const void* ptr_id) final
  {
    return ImGui::GetID(ptr_id);
  }

  // Widgets
  virtual void Text(const char* fmt, ...) IM_PRINTFARGS(1) final
  {
    va_list args;
    va_start(args, fmt);
    ImGui::TextV(fmt, args);
    va_end(args);
  }
  virtual void TextV(const char* fmt, va_list args) final
  {
    return ImGui::TextV(fmt, args);
  }
  virtual void TextColored(const ImVec4& col, const char* fmt, ...)
    IM_PRINTFARGS(2) final
  {
    va_list args;
    va_start(args, fmt);
    ImGui::TextColoredV(col, fmt, args);
    va_end(args);
  }
  virtual void
    TextColoredV(const ImVec4& col, const char* fmt, va_list args) final
  {
    return ImGui::TextColoredV(col, fmt, args);
  }
  virtual void TextDisabled(const char* fmt, ...) IM_PRINTFARGS(1) final
  {
    va_list args;
    va_start(args, fmt);
    ImGui::TextDisabledV(fmt, args);
    va_end(args);
  }
  virtual void TextDisabledV(const char* fmt, va_list args) final
  {
    return ImGui::TextDisabledV(fmt, args);
  }
  virtual void TextWrapped(const char* fmt, ...) IM_PRINTFARGS(1) final
  {
    va_list args;
    va_start(args, fmt);
    ImGui::TextWrappedV(fmt, args);
    va_end(args);
  }
  virtual void TextWrappedV(const char* fmt, va_list args) final
  {
    return ImGui::TextWrappedV(fmt, args);
  }
  virtual void TextUnformatted(const char* text,
                               const char* text_end = NULL) final
  {
    return ImGui::TextUnformatted(text, text_end);
  }
  virtual void LabelText(const char* label, const char* fmt, ...)
    IM_PRINTFARGS(2) final
  {
    va_list args;
    va_start(args, fmt);
    ImGui::LabelTextV(label, fmt, args);
    va_end(args);
  }
  virtual void
    LabelTextV(const char* label, const char* fmt, va_list args) final
  {
    return ImGui::LabelTextV(label, fmt, args);
  }
  virtual void Bullet() final
  {
    return ImGui::Bullet();
  }
  virtual void BulletText(const char* fmt, ...) IM_PRINTFARGS(1) final
  {
    va_list args;
    va_start(args, fmt);
    ImGui::BulletTextV(fmt, args);
    va_end(args);
  }
  virtual void BulletTextV(const char* fmt, va_list args) final
  {
    return ImGui::BulletTextV(fmt, args);
  }
  virtual bool Button(const char* label,
                      const ImVec2& size = ImVec2(0, 0)) final
  {
    return ImGui::Button(label, size);
  }
  virtual bool SmallButton(const char* label) final
  {
    return ImGui::SmallButton(label);
  }
  virtual bool InvisibleButton(const char* str_id, const ImVec2& size) final
  {
    return ImGui::Button(str_id, size);
  }
  virtual void Image(ImTextureID user_texture_id,
                     const ImVec2& size,
                     const ImVec2& uv0 = ImVec2(0, 0),
                     const ImVec2& uv1 = ImVec2(1, 1),
                     const ImVec4& tint_col = ImVec4(1, 1, 1, 1),
                     const ImVec4& border_col = ImVec4(0, 0, 0, 0)) final
  {
    return ImGui::Image(user_texture_id, size, uv0, uv1, tint_col, border_col);
  }
  virtual bool ImageButton(ImTextureID user_texture_id,
                           const ImVec2& size,
                           const ImVec2& uv0 = ImVec2(0, 0),
                           const ImVec2& uv1 = ImVec2(1, 1),
                           int frame_padding = -1,
                           const ImVec4& bg_col = ImVec4(0, 0, 0, 0),
                           const ImVec4& tint_col = ImVec4(1, 1, 1, 1)) final
  {
    return ImGui::ImageButton(
      user_texture_id, size, uv0, uv1, frame_padding, bg_col, tint_col);
  }
  virtual bool CollapsingHeader(const char* label,
                                const char* str_id = NULL,
                                bool display_frame = true,
                                bool default_open = false) final
  {
    return ImGui::CollapsingHeader(label, str_id, display_frame, default_open);
  }
  virtual bool Checkbox(const char* label, bool* v) final
  {
    return ImGui::Checkbox(label, v);
  }
  virtual bool CheckboxFlags(const char* label,
                             unsigned int* flags,
                             unsigned int flags_value) final
  {
    return ImGui::CheckboxFlags(label, flags, flags_value);
  }
  virtual bool RadioButton(const char* label, bool active) final
  {
    return ImGui::RadioButton(label, active);
  }
  virtual bool RadioButton(const char* label, int* v, int v_button) final
  {
    return ImGui::RadioButton(label, v, v_button);
  }
  virtual bool Combo(const char* label,
                     int* current_item,
                     const char** items,
                     int items_count,
                     int height_in_items = -1) final
  {
    return ImGui::Combo(
      label, current_item, items, items_count, height_in_items);
  }
  virtual bool Combo(const char* label,
                     int* current_item,
                     const char* items_separated_by_zeros,
                     int height_in_items = -1) final
  {
    return ImGui::Combo(
      label, current_item, items_separated_by_zeros, height_in_items);
  }
  virtual bool
    Combo(const char* label,
          int* current_item,
          bool (*items_getter)(void* data, int idx, const char** out_text),
          void* data,
          int items_count,
          int height_in_items = -1) final
  {
    return ImGui::Combo(
      label, current_item, items_getter, data, items_count, height_in_items);
  }
  virtual bool ColorButton(const ImVec4& col,
                           bool small_height = false,
                           bool outline_border = true) final
  {
    return ImGui::ColorButton(col, small_height, outline_border);
  }
  virtual bool ColorEdit3(const char* label, float col[3]) final
  {
    return ImGui::ColorEdit3(label, col);
  }
  virtual bool
    ColorEdit4(const char* label, float col[4], bool show_alpha = true) final
  {
    return ImGui::ColorEdit4(label, col, show_alpha);
  }
  virtual void ColorEditMode(ImGuiColorEditMode mode) final
  {
    return ImGui::ColorEditMode(mode);
  }
  virtual void PlotLines(const char* label,
                         const float* values,
                         int values_count,
                         int values_offset = 0,
                         const char* overlay_text = NULL,
                         float scale_min = FLT_MAX,
                         float scale_max = FLT_MAX,
                         ImVec2 graph_size = ImVec2(0, 0),
                         int stride = sizeof(float)) final
  {
    return ImGui::PlotLines(label,
                            values,
                            values_count,
                            values_offset,
                            overlay_text,
                            scale_min,
                            scale_max,
                            graph_size,
                            stride);
  }
  virtual void PlotLines(const char* label,
                         float (*values_getter)(void* data, int idx),
                         void* data,
                         int values_count,
                         int values_offset = 0,
                         const char* overlay_text = NULL,
                         float scale_min = FLT_MAX,
                         float scale_max = FLT_MAX,
                         ImVec2 graph_size = ImVec2(0, 0)) final
  {
    return ImGui::PlotLines(label,
                            values_getter,
                            data,
                            values_count,
                            values_offset,
                            overlay_text,
                            scale_min,
                            scale_max,
                            graph_size);
  }
  virtual void PlotHistogram(const char* label,
                             const float* values,
                             int values_count,
                             int values_offset = 0,
                             const char* overlay_text = NULL,
                             float scale_min = FLT_MAX,
                             float scale_max = FLT_MAX,
                             ImVec2 graph_size = ImVec2(0, 0),
                             int stride = sizeof(float)) final
  {
    return ImGui::PlotHistogram(label,
                                values,
                                values_count,
                                values_offset,
                                overlay_text,
                                scale_min,
                                scale_max,
                                graph_size,
                                stride);
  }
  virtual void PlotHistogram(const char* label,
                             float (*values_getter)(void* data, int idx),
                             void* data,
                             int values_count,
                             int values_offset = 0,
                             const char* overlay_text = NULL,
                             float scale_min = FLT_MAX,
                             float scale_max = FLT_MAX,
                             ImVec2 graph_size = ImVec2(0, 0)) final
  {
    return ImGui::PlotHistogram(label,
                                values_getter,
                                data,
                                values_count,
                                values_offset,
                                overlay_text,
                                scale_min,
                                scale_max,
                                graph_size);
  }

  // Widgets: Drags (tip: ctrl+click on a drag box to input text)
  virtual bool DragFloat(const char* label,
                         float* v,
                         float v_speed = 1.0f,
                         float v_min = 0.0f,
                         float v_max = 0.0f,
                         const char* display_format = "%.3f",
                         float power = 1.0f) final
  {
    return ImGui::DragFloat(
      label, v, v_speed, v_min, v_max, display_format, power);
  }
  virtual bool DragFloat2(const char* label,
                          float v[2],
                          float v_speed = 1.0f,
                          float v_min = 0.0f,
                          float v_max = 0.0f,
                          const char* display_format = "%.3f",
                          float power = 1.0f) final
  {
    return ImGui::DragFloat2(
      label, v, v_speed, v_min, v_max, display_format, power);
  }
  virtual bool DragFloat3(const char* label,
                          float v[3],
                          float v_speed = 1.0f,
                          float v_min = 0.0f,
                          float v_max = 0.0f,
                          const char* display_format = "%.3f",
                          float power = 1.0f) final
  {
    return ImGui::DragFloat3(
      label, v, v_speed, v_min, v_max, display_format, power);
  }
  virtual bool DragFloat4(const char* label,
                          float v[4],
                          float v_speed = 1.0f,
                          float v_min = 0.0f,
                          float v_max = 0.0f,
                          const char* display_format = "%.3f",
                          float power = 1.0f) final
  {
    return ImGui::DragFloat4(
      label, v, v_speed, v_min, v_max, display_format, power);
  }
  virtual bool DragFloatRange2(const char* label,
                               float* v_current_min,
                               float* v_current_max,
                               float v_speed = 1.0f,
                               float v_min = 0.0f,
                               float v_max = 0.0f,
                               const char* display_format = "%.3f",
                               const char* display_format_max = NULL,
                               float power = 1.0f) final
  {
    return ImGui::DragFloatRange2(label,
                                  v_current_min,
                                  v_current_max,
                                  v_speed,
                                  v_min,
                                  v_max,
                                  display_format,
                                  display_format_max,
                                  power);
  }
  virtual bool DragInt(const char* label,
                       int* v,
                       float v_speed = 1.0f,
                       int v_min = 0,
                       int v_max = 0,
                       const char* display_format = "%.0f") final
  {
    return ImGui::DragInt(label, v, v_speed, v_min, v_max, display_format);
  }
  virtual bool DragInt2(const char* label,
                        int v[2],
                        float v_speed = 1.0f,
                        int v_min = 0,
                        int v_max = 0,
                        const char* display_format = "%.0f") final
  {
    return ImGui::DragInt2(label, v, v_speed, v_min, v_max, display_format);
  }
  virtual bool DragInt3(const char* label,
                        int v[3],
                        float v_speed = 1.0f,
                        int v_min = 0,
                        int v_max = 0,
                        const char* display_format = "%.0f") final
  {
    return ImGui::DragInt3(label, v, v_speed, v_min, v_max, display_format);
  }
  virtual bool DragInt4(const char* label,
                        int v[4],
                        float v_speed = 1.0f,
                        int v_min = 0,
                        int v_max = 0,
                        const char* display_format = "%.0f") final
  {
    return ImGui::DragInt4(label, v, v_speed, v_min, v_max, display_format);
  }
  virtual bool DragIntRange2(const char* label,
                             int* v_current_min,
                             int* v_current_max,
                             float v_speed = 1.0f,
                             int v_min = 0,
                             int v_max = 0,
                             const char* display_format = "%.0f",
                             const char* display_format_max = NULL) final
  {
    return ImGui::DragIntRange2(label,
                                v_current_min,
                                v_current_max,
                                v_speed,
                                v_min,
                                v_max,
                                display_format,
                                display_format_max);
  }

  // Widgets: Input
  virtual bool InputText(const char* label,
                         char* buf,
                         size_t buf_size,
                         ImGuiInputTextFlags flags = 0,
                         ImGuiTextEditCallback callback = NULL,
                         void* user_data = NULL) final
  {
    return ImGui::InputText(label, buf, buf_size, flags, callback, user_data);
  }
  virtual bool InputTextMultiline(const char* label,
                                  char* buf,
                                  size_t buf_size,
                                  const ImVec2& size = ImVec2(0, 0),
                                  ImGuiInputTextFlags flags = 0,
                                  ImGuiTextEditCallback callback = NULL,
                                  void* user_data = NULL) final
  {
    return ImGui::InputTextMultiline(
      label, buf, buf_size, size, flags, callback, user_data);
  }
  virtual bool InputFloat(const char* label,
                          float* v,
                          float step = 0.0f,
                          float step_fast = 0.0f,
                          int decimal_precision = -1,
                          ImGuiInputTextFlags extra_flags = 0) final
  {
    return ImGui::InputFloat(
      label, v, step, step_fast, decimal_precision, extra_flags);
  }
  virtual bool InputFloat2(const char* label,
                           float v[2],
                           int decimal_precision = -1,
                           ImGuiInputTextFlags extra_flags = 0) final
  {
    return ImGui::InputFloat2(label, v, decimal_precision, extra_flags);
  }
  virtual bool InputFloat3(const char* label,
                           float v[3],
                           int decimal_precision = -1,
                           ImGuiInputTextFlags extra_flags = 0) final
  {
    return ImGui::InputFloat3(label, v, decimal_precision, extra_flags);
  }
  virtual bool InputFloat4(const char* label,
                           float v[4],
                           int decimal_precision = -1,
                           ImGuiInputTextFlags extra_flags = 0) final
  {
    return ImGui::InputFloat4(label, v, decimal_precision, extra_flags);
  }
  virtual bool InputInt(const char* label,
                        int* v,
                        int step = 1,
                        int step_fast = 100,
                        ImGuiInputTextFlags extra_flags = 0) final
  {
    return ImGui::InputInt(label, v, step, step_fast, extra_flags);
  }
  virtual bool InputInt2(const char* label,
                         int v[2],
                         ImGuiInputTextFlags extra_flags = 0) final
  {
    return ImGui::InputInt2(label, v, extra_flags);
  }
  virtual bool InputInt3(const char* label,
                         int v[3],
                         ImGuiInputTextFlags extra_flags = 0) final
  {
    return ImGui::InputInt3(label, v, extra_flags);
  }
  virtual bool InputInt4(const char* label,
                         int v[4],
                         ImGuiInputTextFlags extra_flags = 0) final
  {
    return ImGui::InputInt4(label, v, extra_flags);
  }

  // Widgets: Sliders
  virtual bool SliderFloat(const char* label,
                           float* v,
                           float v_min,
                           float v_max,
                           const char* display_format = "%.3f",
                           float power = 1.0f) final
  {
    return ImGui::SliderFloat(label, v, v_min, v_max, display_format, power);
  }
  virtual bool SliderFloat2(const char* label,
                            float v[2],
                            float v_min,
                            float v_max,
                            const char* display_format = "%.3f",
                            float power = 1.0f) final
  {
    return ImGui::SliderFloat2(label, v, v_min, v_max, display_format, power);
  }
  virtual bool SliderFloat3(const char* label,
                            float v[3],
                            float v_min,
                            float v_max,
                            const char* display_format = "%.3f",
                            float power = 1.0f) final
  {
    return ImGui::SliderFloat3(label, v, v_min, v_max, display_format, power);
  }
  virtual bool SliderFloat4(const char* label,
                            float v[4],
                            float v_min,
                            float v_max,
                            const char* display_format = "%.3f",
                            float power = 1.0f) final
  {
    return ImGui::SliderFloat4(label, v, v_min, v_max, display_format, power);
  }
  virtual bool SliderAngle(const char* label,
                           float* v_rad,
                           float v_degrees_min = -360.0f,
                           float v_degrees_max = +360.0f) final
  {
    return ImGui::SliderFloat4(label, v_rad, v_degrees_min, v_degrees_max);
  }
  virtual bool SliderInt(const char* label,
                         int* v,
                         int v_min,
                         int v_max,
                         const char* display_format = "%.0f") final
  {
    return ImGui::SliderInt(label, v, v_min, v_max, display_format);
  }
  virtual bool SliderInt2(const char* label,
                          int v[2],
                          int v_min,
                          int v_max,
                          const char* display_format = "%.0f") final
  {
    return ImGui::SliderInt2(label, v, v_min, v_max, display_format);
  }
  virtual bool SliderInt3(const char* label,
                          int v[3],
                          int v_min,
                          int v_max,
                          const char* display_format = "%.0f") final
  {
    return ImGui::SliderInt3(label, v, v_min, v_max, display_format);
  }
  virtual bool SliderInt4(const char* label,
                          int v[4],
                          int v_min,
                          int v_max,
                          const char* display_format = "%.0f") final
  {
    return ImGui::SliderInt4(label, v, v_min, v_max, display_format);
  }
  virtual bool VSliderFloat(const char* label,
                            const ImVec2& size,
                            float* v,
                            float v_min,
                            float v_max,
                            const char* display_format = "%.3f",
                            float power = 1.0f) final
  {
    return ImGui::VSliderFloat(
      label, size, v, v_min, v_max, display_format, power);
  }
  virtual bool VSliderInt(const char* label,
                          const ImVec2& size,
                          int* v,
                          int v_min,
                          int v_max,
                          const char* display_format = "%.0f") final
  {
    return ImGui::VSliderInt(label, size, v, v_min, v_max, display_format);
  }

  // Widgets: Trees
  virtual bool TreeNode(const char* str_label_id) final
  {
    return ImGui::TreeNode(str_label_id);
  }
  virtual bool TreeNode(const char* str_id, const char* fmt, ...)
    IM_PRINTFARGS(2) final
  {
    va_list args;
    va_start(args, fmt);
    auto const ret = ImGui::TreeNodeV(str_id, fmt, args);
    va_end(args);
    return ret;
  }
  virtual bool TreeNode(const void* ptr_id, const char* fmt, ...)
    IM_PRINTFARGS(2) final
  {
    va_list args;
    va_start(args, fmt);
    auto const ret = ImGui::TreeNodeV(ptr_id, fmt, args);
    va_end(args);
    return ret;
  }
  virtual bool
    TreeNodeV(const char* str_id, const char* fmt, va_list args) final
  {
    return ImGui::TreeNodeV(str_id, fmt, args);
  }
  virtual bool
    TreeNodeV(const void* ptr_id, const char* fmt, va_list args) final
  {
    return ImGui::TreeNodeV(ptr_id, fmt, args);
  }
  virtual void TreePush(const char* str_id = NULL) final
  {
    return ImGui::TreePush(str_id);
  }
  virtual void TreePush(const void* ptr_id = NULL) final
  {
    return ImGui::TreePush(ptr_id);
  }
  virtual void TreePop() final
  {
    return ImGui::TreePop();
  }
  virtual void SetNextTreeNodeOpened(bool opened, ImGuiSetCond cond = 0) final
  {
    return ImGui::SetNextTreeNodeOpened(opened, cond);
  }

  // Widgets: Selectable / Lists
  virtual bool Selectable(const char* label,
                          bool selected = false,
                          ImGuiSelectableFlags flags = 0,
                          const ImVec2& size = ImVec2(0, 0)) final
  {
    return ImGui::Selectable(label, selected, flags, size);
  }
  virtual bool Selectable(const char* label,
                          bool* p_selected,
                          ImGuiSelectableFlags flags = 0,
                          const ImVec2& size = ImVec2(0, 0)) final
  {
    return ImGui::Selectable(label, p_selected, flags, size);
  }
  virtual bool ListBox(const char* label,
                       int* current_item,
                       const char** items,
                       int items_count,
                       int height_in_items = -1) final
  {
    return ImGui::ListBox(
      label, current_item, items, items_count, height_in_items);
  }
  virtual bool
    ListBox(const char* label,
            int* current_item,
            bool (*items_getter)(void* data, int idx, const char** out_text),
            void* data,
            int items_count,
            int height_in_items = -1) final
  {
    return ImGui::ListBox(
      label, current_item, items_getter, data, items_count, height_in_items);
  }
  virtual bool ListBoxHeader(const char* label,
                             const ImVec2& size = ImVec2(0, 0)) final
  {
    return ImGui::ListBoxHeader(label, size);
  }
  virtual bool ListBoxHeader(const char* label,
                             int items_count,
                             int height_in_items = -1) final
  {
    return ImGui::ListBoxHeader(label, items_count, height_in_items);
  }
  virtual void ListBoxFooter() final
  {
    return ImGui::ListBoxFooter();
  }

  // Widgets: Value() Helpers
  virtual void Value(const char* prefix, bool b) final
  {
    return ImGui::Value(prefix, b);
  }
  virtual void Value(const char* prefix, int v) final
  {
    return ImGui::Value(prefix, v);
  }
  virtual void Value(const char* prefix, unsigned int v) final
  {
    return ImGui::Value(prefix, v);
  }
  virtual void
    Value(const char* prefix, float v, const char* float_format = NULL) final
  {
    return ImGui::Value(prefix, v, float_format);
  }
#if 0
  virtual void Color(const char* prefix, const ImVec4& v) final
  {
    return ImGui::Color(prefix, v);
  }
  virtual void Color(const char* prefix, unsigned int v) final
  {
    return ImGui::Color(prefix, v);
  }
#endif

  // Tooltip
  virtual void SetTooltip(const char* fmt, ...) IM_PRINTFARGS(1) final
  {
    va_list args;
    va_start(args, fmt);
    ImGui::SetTooltipV(fmt, args);
    va_end(args);
  }
  virtual void SetTooltipV(const char* fmt, va_list args) final
  {
    return ImGui::SetTooltipV(fmt, args);
  }
  virtual void BeginTooltip() final
  {
    return ImGui::BeginTooltip();
  }
  virtual void EndTooltip() final
  {
    return ImGui::EndTooltip();
  }

  // Menus
  virtual bool BeginMainMenuBar() final
  {
    return ImGui::BeginMainMenuBar();
  }
  virtual void EndMainMenuBar() final
  {
    return ImGui::EndMainMenuBar();
  }
  virtual bool BeginMenuBar() final
  {
    return ImGui::BeginMenuBar();
  }
  virtual void EndMenuBar() final
  {
    return ImGui::EndMenuBar();
  }
  virtual bool BeginMenu(const char* label, bool enabled = true) final
  {
    return ImGui::BeginMenu(label, enabled);
  }
  virtual void EndMenu() final
  {
    return ImGui::EndMenu();
  }
  virtual bool MenuItem(const char* label,
                        const char* shortcut = NULL,
                        bool selected = false,
                        bool enabled = true) final
  {
    return ImGui::MenuItem(label, shortcut, selected, enabled);
  }
  virtual bool MenuItem(const char* label,
                        const char* shortcut,
                        bool* p_selected,
                        bool enabled = true) final
  {
    return ImGui::MenuItem(label, shortcut, p_selected, enabled);
  }

  // Popup
  virtual void OpenPopup(const char* str_id) final
  {
    return ImGui::OpenPopup(str_id);
  }
  virtual bool BeginPopup(const char* str_id) final
  {
    return ImGui::BeginPopup(str_id);
  }
  virtual bool BeginPopupModal(const char* name,
                               bool* p_opened = NULL,
                               ImGuiWindowFlags extra_flags = 0) final
  {
    return ImGui::BeginPopupModal(name, p_opened, extra_flags);
  }
  virtual bool BeginPopupContextItem(const char* str_id,
                                     int mouse_button = 1) final
  {
    return ImGui::BeginPopupContextItem(str_id, mouse_button);
  }
  virtual bool BeginPopupContextWindow(bool also_over_items = true,
                                       const char* str_id = NULL,
                                       int mouse_button = 1) final
  {
    return ImGui::BeginPopupContextWindow(
      also_over_items, str_id, mouse_button);
  }
  virtual bool BeginPopupContextVoid(const char* str_id = NULL,
                                     int mouse_button = 1) final
  {
    return ImGui::BeginPopupContextVoid(str_id, mouse_button);
  }
  virtual void EndPopup() final
  {
    return ImGui::EndPopup();
  }
  virtual void CloseCurrentPopup() final
  {
    return ImGui::CloseCurrentPopup();
  }

  // Logging
  virtual void LogToTTY(int max_depth = -1) final
  {
    return ImGui::LogToTTY(max_depth);
  }
  virtual void LogToFile(int max_depth = -1, const char* filename = NULL) final
  {
    return ImGui::LogToFile(max_depth, filename);
  }
  virtual void LogToClipboard(int max_depth = -1) final
  {
    return ImGui::LogToClipboard(max_depth);
  }
  virtual void LogFinish() final
  {
    return ImGui::LogFinish();
  }
  virtual void LogButtons() final
  {
    return ImGui::LogButtons();
  }
  virtual void LogText(const char* fmt, ...) IM_PRINTFARGS(1) final
  {
    // TODO: Implement this. Needs LogTextV?
    HADESMEM_DETAIL_TRACE_A("WARNING! Unimplemented API wrapper called.");
    (void)fmt;
  }

  // Utilities
  virtual bool IsItemHovered() final
  {
    return ImGui::IsItemHovered();
  }
  virtual bool IsItemHoveredRect() final
  {
    return ImGui::IsItemHoveredRect();
  }
  virtual bool IsItemActive() final
  {
    return ImGui::IsItemActive();
  }
  virtual bool IsItemVisible() final
  {
    return ImGui::IsItemVisible();
  }
  virtual bool IsAnyItemHovered() final
  {
    return ImGui::IsAnyItemHovered();
  }
  virtual bool IsAnyItemActive() final
  {
    return ImGui::IsAnyItemActive();
  }
  virtual ImVec2 GetItemRectMin() final
  {
    return ImGui::GetItemRectMin();
  }
  virtual ImVec2 GetItemRectMax() final
  {
    return ImGui::GetItemRectMax();
  }
  virtual ImVec2 GetItemRectSize() final
  {
    return ImGui::GetItemRectSize();
  }
  virtual bool IsWindowHovered() final
  {
    return ImGui::IsWindowHovered();
  }
  virtual bool IsWindowFocused() final
  {
    return ImGui::IsWindowFocused();
  }
  virtual bool IsRootWindowFocused() final
  {
    return ImGui::IsRootWindowFocused();
  }
  virtual bool IsRootWindowOrAnyChildFocused() final
  {
    return ImGui::IsRootWindowOrAnyChildFocused();
  }
  virtual bool IsRectVisible(const ImVec2& size) final
  {
    return ImGui::IsRectVisible(size);
  }
  virtual bool IsPosHoveringAnyWindow(const ImVec2& pos) final
  {
    return ImGui::IsPosHoveringAnyWindow(pos);
  }
  virtual float GetTime() final
  {
    return ImGui::GetTime();
  }
  virtual int GetFrameCount() final
  {
    return ImGui::GetFrameCount();
  }
  virtual const char* GetStyleColName(ImGuiCol idx) final
  {
    return ImGui::GetStyleColName(idx);
  }
  virtual ImVec2 CalcItemRectClosestPoint(const ImVec2& pos,
                                          bool on_edge = false,
                                          float outward = +0.0f) final
  {
    return ImGui::CalcItemRectClosestPoint(pos, on_edge, outward);
  }
  virtual ImVec2 CalcTextSize(const char* text,
                              const char* text_end = NULL,
                              bool hide_text_after_double_hash = false,
                              float wrap_width = -1.0f) final
  {
    return ImGui::CalcTextSize(
      text, text_end, hide_text_after_double_hash, wrap_width);
  }
  virtual void CalcListClipping(int items_count,
                                float items_height,
                                int* out_items_display_start,
                                int* out_items_display_end) final
  {
    return ImGui::CalcListClipping(items_count,
                                   items_height,
                                   out_items_display_start,
                                   out_items_display_end);
  }
  virtual bool BeginChildFrame(ImGuiID id,
                               const ImVec2& size,
                               ImGuiWindowFlags extra_flags = 0) final
  {
    return ImGui::BeginChildFrame(id, size, extra_flags);
  }
  virtual void EndChildFrame() final
  {
    return ImGui::EndChildFrame();
  }

  virtual ImVec4 ColorConvertU32ToFloat4(ImU32 in) final
  {
    return ImGui::ColorConvertU32ToFloat4(in);
  }
  virtual ImU32 ColorConvertFloat4ToU32(const ImVec4& in) final
  {
    return ImGui::ColorConvertFloat4ToU32(in);
  }
  virtual void ColorConvertRGBtoHSV(
    float r, float g, float b, float& out_h, float& out_s, float& out_v) final
  {
    return ImGui::ColorConvertRGBtoHSV(r, g, b, out_h, out_s, out_v);
  }
  virtual void ColorConvertHSVtoRGB(
    float h, float s, float v, float& out_r, float& out_g, float& out_b) final
  {
    return ImGui::ColorConvertHSVtoRGB(h, s, v, out_r, out_g, out_b);
  }

  // Inputs
  virtual int GetKeyIndex(ImGuiKey key) final
  {
    return ImGui::GetKeyIndex(key);
  }
  virtual bool IsKeyDown(int key_index) final
  {
    return ImGui::IsKeyDown(key_index);
  }
  virtual bool IsKeyPressed(int key_index, bool repeat = true) final
  {
    return ImGui::IsKeyPressed(key_index, repeat);
  }
  virtual bool IsKeyReleased(int key_index) final
  {
    return ImGui::IsKeyReleased(key_index);
  }
  virtual bool IsMouseDown(int button) final
  {
    return ImGui::IsMouseDown(button);
  }
  virtual bool IsMouseClicked(int button, bool repeat = false) final
  {
    return ImGui::IsMouseClicked(button, repeat);
  }
  virtual bool IsMouseDoubleClicked(int button) final
  {
    return ImGui::IsMouseDoubleClicked(button);
  }
  virtual bool IsMouseReleased(int button) final
  {
    return ImGui::IsMouseReleased(button);
  }
  virtual bool IsMouseHoveringWindow() final
  {
    return ImGui::IsMouseHoveringWindow();
  }
  virtual bool IsMouseHoveringAnyWindow() final
  {
    return ImGui::IsMouseHoveringAnyWindow();
  }
  virtual bool IsMouseHoveringRect(const ImVec2& pos_min,
                                   const ImVec2& pos_max,
                                   bool clip = true) final
  {
    return ImGui::IsMouseHoveringRect(pos_min, pos_max, clip);
  }
  virtual bool IsMouseDragging(int button = 0,
                               float lock_threshold = -1.0f) final
  {
    return ImGui::IsMouseDragging(button, lock_threshold);
  }
  virtual ImVec2 GetMousePos() final
  {
    return ImGui::GetMousePos();
  }
  virtual ImVec2 GetMousePosOnOpeningCurrentPopup() final
  {
    return ImGui::GetMousePosOnOpeningCurrentPopup();
  }
  virtual ImVec2 GetMouseDragDelta(int button = 0,
                                   float lock_threshold = -1.0f) final
  {
    return ImGui::GetMouseDragDelta(button, lock_threshold);
  }
  virtual void ResetMouseDragDelta(int button = 0) final
  {
    return ImGui::ResetMouseDragDelta(button);
  }
  virtual ImGuiMouseCursor GetMouseCursor() final
  {
    return ImGui::GetMouseCursor();
  }
  virtual void SetMouseCursor(ImGuiMouseCursor type) final
  {
    return ImGui::SetMouseCursor(type);
  }
  virtual void CaptureKeyboardFromApp() final
  {
    return ImGui::CaptureKeyboardFromApp();
  }
  virtual void CaptureMouseFromApp() final
  {
    return ImGui::CaptureMouseFromApp();
  }

  virtual void* MemAlloc(size_t sz) final
  {
    return ImGui::MemAlloc(sz);
  }
  virtual void MemFree(void* ptr) final
  {
    return ImGui::MemFree(ptr);
  }
  virtual const char* GetClipboardText() final
  {
    return ImGui::GetClipboardText();
  }
  virtual void SetClipboardText(const char* text) final
  {
    return ImGui::SetClipboardText(text);
  }

  // Internal state/context access
  virtual const char* GetVersion() final
  {
    return ImGui::GetVersion();
  }
  virtual void* GetInternalState() final
  {
    return ImGui::GetInternalState();
  }
  virtual size_t GetInternalStateSize() final
  {
    return ImGui::GetInternalStateSize();
  }
  virtual void SetInternalState(void* state, bool construct = false) final
  {
    return ImGui::SetInternalState(state, construct);
  }
};

std::string& GetPluginPathTw()
{
  static std::string path;
  return path;
}

void OnInitializeImguiGui(hadesmem::cerberus::RenderApi api, void* device)
{
  if (ImguiInitializedAny())
  {
    HADESMEM_DETAIL_TRACE_A("WARNING! Imgui is already initialized. Skipping.");
    return;
  }

  HADESMEM_DETAIL_TRACE_A("Initializing Imgui.");

  auto& window = hadesmem::cerberus::GetWindowInterface();

  switch (api)
  {
  case hadesmem::cerberus::RenderApi::kD3D9:
    ImGui_ImplDX9_Init(window.GetCurrentWindow(),
                       static_cast<IDirect3DDevice9*>(device));
    break;

  case hadesmem::cerberus::RenderApi::kD3D10:
  {
    ImGui_ImplDX10_Init(window.GetCurrentWindow(),
                        static_cast<ID3D10Device*>(device));
    break;
  }

  case hadesmem::cerberus::RenderApi::kD3D11:
  {
    ID3D11DeviceContext* device_context = nullptr;
    auto const device_ = static_cast<ID3D11Device*>(device);
    device_->GetImmediateContext(&device_context);
    ImGui_ImplDX11_Init(window.GetCurrentWindow(), device_, device_context);
    break;
  }

  case hadesmem::cerberus::RenderApi::kOpenGL32:
    // TODO: Add this. Imgui supports it, but uses GLFW unfortunately so we will
    // need to re-write it without the dependencies.
    HADESMEM_DETAIL_TRACE_A(
      "WARNING! Currently unsupported render API (OpenGL32).");
    return;

  default:
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
  }

  SetImguiInitialized(api, true);

  HADESMEM_DETAIL_TRACE_A("Calling Imgui initialization callbacks.");

  auto& callbacks = GetOnImguiInitializeCallbacks();
  callbacks.Run(&hadesmem::cerberus::GetImguiInterface());
}

void OnCleanupImguiGui(hadesmem::cerberus::RenderApi api)
{
  if (!GetImguiInitialized(api))
  {
    return;
  }

  HADESMEM_DETAIL_TRACE_A("Calling Imgui cleanup callbacks.");

  auto& callbacks = GetOnImguiCleanupCallbacks();
  callbacks.Run(&hadesmem::cerberus::GetImguiInterface());

  HADESMEM_DETAIL_TRACE_A("Cleaning up Imgui.");

  switch (api)
  {
  case hadesmem::cerberus::RenderApi::kD3D9:
    HADESMEM_DETAIL_TRACE_A("Cleaning up for D3D9.");
    ImGui_ImplDX9_Shutdown();
    break;

  case hadesmem::cerberus::RenderApi::kD3D10:
    HADESMEM_DETAIL_TRACE_A("Cleaning up for D3D10.");
    ImGui_ImplDX10_Shutdown();
    break;

  case hadesmem::cerberus::RenderApi::kD3D11:
    HADESMEM_DETAIL_TRACE_A("Cleaning up for D3D11.");
    ImGui_ImplDX11_Shutdown();
    break;

  case hadesmem::cerberus::RenderApi::kOpenGL32:
    break;

  default:
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
  }

  HADESMEM_DETAIL_TRACE_A("Clearing Imgui initialization state.");

  SetImguiInitialized(api, false);
}

bool& GetAllImguiVisibility() noexcept
{
  static bool visible{false};
  return visible;
}

void SetAllImguiVisibility(bool visible, bool /*old_visible*/)
{
  GetAllImguiVisibility() = visible;
}

// TODO: Remove duplication between this and CanonicalizePluginPath
// TODO: Handle casing and canonicalization issues manually, we can't rely on
// the FS here (or can we? perhaps the map key should be a FileID instead of a
// path?).
std::wstring CanonicalizeScriptPath(std::wstring path)
{
  if (hadesmem::detail::IsPathRelative(path))
  {
    path =
      hadesmem::detail::CombinePath(hadesmem::detail::GetSelfDirPath(), path);
  }
  path = hadesmem::detail::MakeExtendedPath(path);
  return path;
}

void HandleInputQueueEntry(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (!ImguiInitializedAny() || !GetAllImguiVisibility())
  {
    return;
  }

  auto& window_interface = hadesmem::cerberus::GetWindowInterface();
  if (hwnd == window_interface.GetCurrentWindow())
  {
    // Simply calling the DX11 implementation unconditionally because it's
    // identical to the DX9 one.
    ImGui_ImplDX11_WndProcHandler(hwnd, msg, wparam, lparam);
  }
}

void OnFrameImgui(hadesmem::cerberus::RenderApi api, void* /*device*/)
{
  if (!ImguiInitializedAny() || !GetAllImguiVisibility())
  {
    return;
  }

  switch (api)
  {
  case hadesmem::cerberus::RenderApi::kD3D9:
    ImGui_ImplDX9_NewFrame();
    break;

  case hadesmem::cerberus::RenderApi::kD3D10:
    ImGui_ImplDX10_NewFrame();
    break;

  case hadesmem::cerberus::RenderApi::kD3D11:
    ImGui_ImplDX11_NewFrame();
    break;

  case hadesmem::cerberus::RenderApi::kOpenGL32:
    break;

  default:
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
  }

  // Move this state somwhere we can properly manage its lifetime.
  static bool show_log_window = false;
  static bool show_console_window = false;

  auto& imgui = hadesmem::cerberus::GetImguiInterface();

  ImGui::SetNextWindowSize(ImVec2(320, 250), ImGuiSetCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(15, 15), ImGuiSetCond_FirstUseEver);
  if (ImGui::Begin("Cerberus"))
  {
    ImGui::Text("Cerberus Version: %s", HADESMEM_VERSION_STRING);
    ImGui::Text("ImGui Version: %s", ImGui::GetVersion());
    ImGui::Text("Renderer: %s",
                hadesmem::cerberus::GetRenderApiName(api).c_str());
    // TODO: Figure out why this differs from the steam overlay.
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::Separator();

    static char plugin_buf[MAX_PATH + 1] = "";
    ImGui::InputText("Plugin", plugin_buf, sizeof(plugin_buf));
    if (ImGui::Button("Load"))
    {
      imgui.LogFormat("[Info]: Loading plugin. Name: [%s].", plugin_buf);

      try
      {
        // TODO: Make this fail on an empty string.
        hadesmem::cerberus::LoadPlugin(
          hadesmem::detail::MultiByteToWideChar(plugin_buf));
      }
      catch (...)
      {
        imgui.LogFormat(
          "[Error]: %s",
          boost::current_exception_diagnostic_information().c_str());
      }
    }

    // TODO: Add an easier way to unload individual plugins (dropdown list?).
    ImGui::SameLine();
    if (ImGui::Button("Unload"))
    {
      imgui.LogFormat("[Info]: Unloading plugin. Name: [%s].", plugin_buf);
      
      try
      {
        // TODO: Make this actually error out when we can't find the module.
        hadesmem::cerberus::UnloadPlugin(
          hadesmem::detail::MultiByteToWideChar(plugin_buf));
      }
      catch (...)
      {
        imgui.LogFormat(
          "[Error]: %s",
          boost::current_exception_diagnostic_information().c_str());
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Unload All"))
    {
      imgui.Log("[Info]: Unloading all plugins.");

      try
      {
        hadesmem::cerberus::UnloadPlugins();
      }
      catch (...)
      {
        imgui.LogFormat(
          "[Error]: %s",
          boost::current_exception_diagnostic_information().c_str());
      }
    }

    ImGui::Separator();

    auto& scripts = hadesmem::cerberus::GetChaiScriptScripts();

    static char script_buf[MAX_PATH + 1] = "";
    ImGui::InputText("Script", script_buf, sizeof(script_buf));
    if (ImGui::Button("Start"))
    {
      auto const script_path =
        hadesmem::detail::WideCharToMultiByte(CanonicalizeScriptPath(
          hadesmem::detail::MultiByteToWideChar(script_buf)));
      if (scripts.find(hadesmem::detail::ToUpperOrdinal(script_path)) ==
          std::end(scripts))
      {
        scripts.emplace(hadesmem::detail::ToUpperOrdinal(script_path),
                        hadesmem::cerberus::ChaiScriptScript(script_path));
      }
      else
      {
        imgui.LogFormat("[Error]: Failed to start already running script. "
                        "Name: [%s]. Path: [%s].",
                        script_buf,
                        script_path.c_str());
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
      auto const script_path =
        hadesmem::detail::WideCharToMultiByte(CanonicalizeScriptPath(
          hadesmem::detail::MultiByteToWideChar(script_buf)));
      auto const script_iter =
        scripts.find(hadesmem::detail::ToUpperOrdinal(script_path));
      if (script_iter != std::end(scripts))
      {
        scripts.erase(script_iter);
      }
      else
      {
        imgui.LogFormat(
          "[Error]: Failed to find script. Name: [%s]. Path: [%s].",
          script_buf,
          script_path.c_str());
      }
    }

    ImGui::Separator();

    if (ImGui::Button("Dump PE"))
    {
      try
      {
        imgui.Log("[Info]: Dumping PE files.");
        hadesmem::detail::DumpMemory();
      }
      catch (...)
      {
        imgui.LogFormat(
          "[Error]: %s",
          boost::current_exception_diagnostic_information().c_str());
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("Debug Break"))
    {
      imgui.Log("[Info]: Performing debug break.");
      __debugbreak();
    }

    ImGui::SameLine();
    if (ImGui::Button("Eject"))
    {
      imgui.Log("[Info]: Ejecting.");
      auto const eject = [](LPVOID /*arg*/) -> DWORD
      {
        Free();
        FreeLibraryAndExitThread(hadesmem::detail::GetHandleToSelf(), 0);
      };
      ::CreateThread(nullptr, 0, eject, nullptr, 0, nullptr);
    }

    ImGui::Separator();

    if (ImGui::Button("Console"))
    {
      show_console_window ^= 1;
    }

    ImGui::SameLine();
    if (ImGui::Button("Log"))
    {
      show_log_window ^= 1;
    }
  }
  ImGui::End();

  if (show_console_window)
  {
    auto& console = hadesmem::cerberus::GetImGuiConsoleWindow();
    console.Draw("Console", &show_console_window);
  }

  if (show_log_window)
  {
    auto& log = hadesmem::cerberus::GetImGuiLogWindow();
    log.Draw("Log", &show_log_window);
  }

  auto const& callbacks = GetOnImguiFrameCallbacks();
  callbacks.Run();

  ImGui::Render();
}

// TODO: Remove the need for this. It's awful and causes just as many problems
// as it solves.
void OnUnloadPlugins()
{
  OnCleanupImguiGui(hadesmem::cerberus::RenderApi::kD3D9);
  OnCleanupImguiGui(hadesmem::cerberus::RenderApi::kD3D10);
  OnCleanupImguiGui(hadesmem::cerberus::RenderApi::kD3D11);
  OnCleanupImguiGui(hadesmem::cerberus::RenderApi::kOpenGL32);
}
}

namespace hadesmem
{
namespace cerberus
{
ImguiInterface& GetImguiInterface() noexcept
{
  static ImguiImpl imgui;
  return imgui;
}

void InitializeImgui()
{
  auto& input = GetInputInterface();
  input.RegisterOnInputQueueEntry(HandleInputQueueEntry);

  // No OnResize handler necessary because ImGui calls GetClientRect every
  // frame.
  auto& render = GetRenderInterface();
  render.RegisterOnFrame(OnFrameImgui);
  render.RegisterOnInitializeGui(OnInitializeImguiGui);
  render.RegisterOnCleanupGui(OnCleanupImguiGui);
  render.RegisterOnSetGuiVisibility(SetAllImguiVisibility);

  RegisterOnUnloadPlugins(OnUnloadPlugins);
}
}
}
