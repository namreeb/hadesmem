// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdio>
#include <cstdint>
#include <functional>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/config.hpp>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <imgui/imgui.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

namespace hadesmem
{
namespace cerberus
{
class ImguiInterface;

typedef void OnImguiInitializeCallback(ImguiInterface* imgui);

typedef void OnImguiCleanupCallback(ImguiInterface* imgui);

typedef void OnImguiFrameCallback();

// TODO: Actually expose the ImGui API.
// TODO: Fix casing of this and all other types. (Imgui -> ImGui)
class ImguiInterface
{
public:
  virtual ~ImguiInterface()
  {
  }

  virtual std::size_t RegisterOnInitialize(
    std::function<OnImguiInitializeCallback> const& callback) = 0;

  virtual void UnregisterOnInitialize(std::size_t id) = 0;

  virtual std::size_t RegisterOnCleanup(
    std::function<OnImguiCleanupCallback> const& callback) = 0;

  virtual void UnregisterOnCleanup(std::size_t id) = 0;

  // TODO: Ensure these events are fired after the regular OnFrame events,
  // because in the case of writing a script for a bot or similar, you would
  // want to use regular OnFrame actions to gather your data and run your bot
  // logic, and then run your GUI OnFrame afterwards so you can draw relevant
  // player information, what action was taken, etc...
  virtual std::size_t
    RegisterOnFrame(std::function<OnImguiFrameCallback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;

  // TODO: Replace this with something that can work with any GUI lib. Also add
  // printf-style wrappers.
  virtual void Log(std::string const& s) = 0;

  virtual bool IsInitialized() = 0;

  // Main
  virtual ImGuiIO& GetIO() = 0;
  virtual ImGuiStyle& GetStyle() = 0;
  virtual ImDrawData* GetDrawData() = 0;
  virtual void NewFrame() = 0;
  virtual void Render() = 0;
  virtual void Shutdown() = 0;
  virtual void ShowUserGuide() = 0;
  virtual void ShowStyleEditor(ImGuiStyle* ref = NULL) = 0;
  virtual void ShowTestWindow(bool* opened = NULL) = 0;
  virtual void ShowMetricsWindow(bool* opened = NULL) = 0;

  // Window
  virtual bool Begin(const char* name,
                     bool* p_opened = NULL,
                     ImGuiWindowFlags flags = 0) = 0;
  virtual bool Begin(const char* name,
                     bool* p_opened,
                     const ImVec2& size_on_first_use,
                     float bg_alpha = -1.0f,
                     ImGuiWindowFlags flags = 0) = 0;
  virtual void End() = 0;
  virtual bool BeginChild(const char* str_id,
                          const ImVec2& size = ImVec2(0, 0),
                          bool border = false,
                          ImGuiWindowFlags extra_flags = 0) = 0;
  virtual bool BeginChild(ImGuiID id,
                          const ImVec2& size = ImVec2(0, 0),
                          bool border = false,
                          ImGuiWindowFlags extra_flags = 0) = 0;
  virtual void EndChild() = 0;
  virtual ImVec2 GetContentRegionMax() = 0;
  virtual ImVec2 GetContentRegionAvail() = 0;
  virtual float GetContentRegionAvailWidth() = 0;
  virtual ImVec2 GetWindowContentRegionMin() = 0;
  virtual ImVec2 GetWindowContentRegionMax() = 0;
  virtual float GetWindowContentRegionWidth() = 0;
  virtual ImDrawList* GetWindowDrawList() = 0;
  virtual ImFont* GetWindowFont() = 0;
  virtual float GetWindowFontSize() = 0;
  virtual void SetWindowFontScale(float scale) = 0;
  virtual ImVec2 GetWindowPos() = 0;
  virtual ImVec2 GetWindowSize() = 0;
  virtual float GetWindowWidth() = 0;
  virtual float GetWindowHeight() = 0;
  virtual bool IsWindowCollapsed() = 0;

  virtual void SetNextWindowPos(const ImVec2& pos, ImGuiSetCond cond = 0) = 0;
  virtual void SetNextWindowPosCenter(ImGuiSetCond cond = 0) = 0;
  virtual void SetNextWindowSize(const ImVec2& size, ImGuiSetCond cond = 0) = 0;
  virtual void SetNextWindowContentSize(const ImVec2& size) = 0;
  virtual void SetNextWindowContentWidth(float width) = 0;
  virtual void SetNextWindowCollapsed(bool collapsed,
                                      ImGuiSetCond cond = 0) = 0;
  virtual void SetNextWindowFocus() = 0;
  virtual void SetWindowPos(const ImVec2& pos, ImGuiSetCond cond = 0) = 0;
  virtual void SetWindowSize(const ImVec2& size, ImGuiSetCond cond = 0) = 0;
  virtual void SetWindowCollapsed(bool collapsed, ImGuiSetCond cond = 0) = 0;
  virtual void SetWindowFocus() = 0;
  virtual void SetWindowPos(const char* name,
                            const ImVec2& pos,
                            ImGuiSetCond cond = 0) = 0;
  virtual void SetWindowSize(const char* name,
                             const ImVec2& size,
                             ImGuiSetCond cond = 0) = 0;
  virtual void SetWindowCollapsed(const char* name,
                                  bool collapsed,
                                  ImGuiSetCond cond = 0) = 0;
  virtual void SetWindowFocus(const char* name) = 0;

  virtual float GetScrollX() = 0;
  virtual float GetScrollY() = 0;
  virtual float GetScrollMaxX() = 0;
  virtual float GetScrollMaxY() = 0;
  virtual void SetScrollX(float scroll_x) = 0;
  virtual void SetScrollY(float scroll_y) = 0;
  virtual void SetScrollHere(float center_y_ratio = 0.5f) = 0;
  virtual void SetScrollFromPosY(float pos_y, float center_y_ratio = 0.5f) = 0;
  virtual void SetKeyboardFocusHere(int offset = 0) = 0;
  virtual void SetStateStorage(ImGuiStorage* tree) = 0;
  virtual ImGuiStorage* GetStateStorage() = 0;

  // Parameters stacks (shared)
  virtual void PushFont(ImFont* font) = 0;
  virtual void PopFont() = 0;
  virtual void PushStyleColor(ImGuiCol idx, const ImVec4& col) = 0;
  virtual void PopStyleColor(int count = 1) = 0;
  virtual void PushStyleVar(ImGuiStyleVar idx, float val) = 0;
  virtual void PushStyleVar(ImGuiStyleVar idx, const ImVec2& val) = 0;
  virtual void PopStyleVar(int count = 1) = 0;

  // Parameters stacks (current window)
  virtual void PushItemWidth(float item_width) = 0;
  virtual void PopItemWidth() = 0;
  virtual float CalcItemWidth() = 0;
  virtual void PushTextWrapPos(float wrap_pos_x = 0.0f) = 0;
  virtual void PopTextWrapPos() = 0;
  virtual void PushAllowKeyboardFocus(bool v) = 0;
  virtual void PopAllowKeyboardFocus() = 0;
  virtual void PushButtonRepeat(bool repeat) = 0;
  virtual void PopButtonRepeat() = 0;

  // Cursor / Layout
  virtual void BeginGroup() = 0;
  virtual void EndGroup() = 0;
  virtual void Separator() = 0;
  virtual void SameLine(float local_pos_x = 0.0f, float spacing_w = -1.0f) = 0;
  virtual void Spacing() = 0;
  virtual void Dummy(const ImVec2& size) = 0;
  virtual void Indent() = 0;
  virtual void Unindent() = 0;
  virtual void
    Columns(int count = 1, const char* id = NULL, bool border = true) = 0;
  virtual void NextColumn() = 0;
  virtual int GetColumnIndex() = 0;
  virtual float GetColumnOffset(int column_index = -1) = 0;
  virtual void SetColumnOffset(int column_index, float offset_x) = 0;
  virtual float GetColumnWidth(int column_index = -1) = 0;
  virtual int GetColumnsCount() = 0;
  virtual ImVec2 GetCursorPos() = 0;
  virtual float GetCursorPosX() = 0;
  virtual float GetCursorPosY() = 0;
  virtual void SetCursorPos(const ImVec2& local_pos) = 0;
  virtual void SetCursorPosX(float x) = 0;
  virtual void SetCursorPosY(float y) = 0;
  virtual ImVec2 GetCursorStartPos() = 0;
  virtual ImVec2 GetCursorScreenPos() = 0;
  virtual void SetCursorScreenPos(const ImVec2& pos) = 0;
  virtual void AlignFirstTextHeightToWidgets() = 0;
  virtual float GetTextLineHeight() = 0;
  virtual float GetTextLineHeightWithSpacing() = 0;
  virtual float GetItemsLineHeightWithSpacing() = 0;

  // ID scopes
  virtual void PushID(const char* str_id) = 0;
  virtual void PushID(const char* str_id_begin, const char* str_id_end) = 0;
  virtual void PushID(const void* ptr_id) = 0;
  virtual void PushID(int int_id) = 0;
  virtual void PopID() = 0;
  virtual ImGuiID GetID(const char* str_id) = 0;
  virtual ImGuiID GetID(const char* str_id_begin, const char* str_id_end) = 0;
  virtual ImGuiID GetID(const void* ptr_id) = 0;

  // Widgets
  virtual void Text(const char* fmt, ...) IM_PRINTFARGS(1) = 0;
  virtual void TextV(const char* fmt, va_list args) = 0;
  virtual void TextColored(const ImVec4& col, const char* fmt, ...)
    IM_PRINTFARGS(2) = 0;
  virtual void
    TextColoredV(const ImVec4& col, const char* fmt, va_list args) = 0;
  virtual void TextDisabled(const char* fmt, ...) IM_PRINTFARGS(1) = 0;
  virtual void TextDisabledV(const char* fmt, va_list args) = 0;
  virtual void TextWrapped(const char* fmt, ...) IM_PRINTFARGS(1) = 0;
  virtual void TextWrappedV(const char* fmt, va_list args) = 0;
  virtual void TextUnformatted(const char* text,
                               const char* text_end = NULL) = 0;
  virtual void LabelText(const char* label, const char* fmt, ...)
    IM_PRINTFARGS(2) = 0;
  virtual void LabelTextV(const char* label, const char* fmt, va_list args) = 0;
  virtual void Bullet() = 0;
  virtual void BulletText(const char* fmt, ...) IM_PRINTFARGS(1) = 0;
  virtual void BulletTextV(const char* fmt, va_list args) = 0;
  virtual bool Button(const char* label, const ImVec2& size = ImVec2(0, 0)) = 0;
  virtual bool SmallButton(const char* label) = 0;
  virtual bool InvisibleButton(const char* str_id, const ImVec2& size) = 0;
  virtual void Image(ImTextureID user_texture_id,
                     const ImVec2& size,
                     const ImVec2& uv0 = ImVec2(0, 0),
                     const ImVec2& uv1 = ImVec2(1, 1),
                     const ImVec4& tint_col = ImVec4(1, 1, 1, 1),
                     const ImVec4& border_col = ImVec4(0, 0, 0, 0)) = 0;
  virtual bool ImageButton(ImTextureID user_texture_id,
                           const ImVec2& size,
                           const ImVec2& uv0 = ImVec2(0, 0),
                           const ImVec2& uv1 = ImVec2(1, 1),
                           int frame_padding = -1,
                           const ImVec4& bg_col = ImVec4(0, 0, 0, 0),
                           const ImVec4& tint_col = ImVec4(1, 1, 1, 1)) = 0;
  virtual bool CollapsingHeader(const char* label,
                                const char* str_id = NULL,
                                bool display_frame = true,
                                bool default_open = false) = 0;
  virtual bool Checkbox(const char* label, bool* v) = 0;
  virtual bool CheckboxFlags(const char* label,
                             unsigned int* flags,
                             unsigned int flags_value) = 0;
  virtual bool RadioButton(const char* label, bool active) = 0;
  virtual bool RadioButton(const char* label, int* v, int v_button) = 0;
  virtual bool Combo(const char* label,
                     int* current_item,
                     const char** items,
                     int items_count,
                     int height_in_items = -1) = 0;
  virtual bool Combo(const char* label,
                     int* current_item,
                     const char* items_separated_by_zeros,
                     int height_in_items = -1) = 0; // separate items with \0,
                                                    // end item-list with \0\0
  virtual bool
    Combo(const char* label,
          int* current_item,
          bool (*items_getter)(void* data, int idx, const char** out_text),
          void* data,
          int items_count,
          int height_in_items = -1) = 0;
  virtual bool ColorButton(const ImVec4& col,
                           bool small_height = false,
                           bool outline_border = true) = 0;
  virtual bool ColorEdit3(const char* label, float col[3]) = 0;
  virtual bool
    ColorEdit4(const char* label, float col[4], bool show_alpha = true) = 0;
  virtual void ColorEditMode(ImGuiColorEditMode mode) = 0;
  virtual void PlotLines(const char* label,
                         const float* values,
                         int values_count,
                         int values_offset = 0,
                         const char* overlay_text = NULL,
                         float scale_min = FLT_MAX,
                         float scale_max = FLT_MAX,
                         ImVec2 graph_size = ImVec2(0, 0),
                         int stride = sizeof(float)) = 0;
  virtual void PlotLines(const char* label,
                         float (*values_getter)(void* data, int idx),
                         void* data,
                         int values_count,
                         int values_offset = 0,
                         const char* overlay_text = NULL,
                         float scale_min = FLT_MAX,
                         float scale_max = FLT_MAX,
                         ImVec2 graph_size = ImVec2(0, 0)) = 0;
  virtual void PlotHistogram(const char* label,
                             const float* values,
                             int values_count,
                             int values_offset = 0,
                             const char* overlay_text = NULL,
                             float scale_min = FLT_MAX,
                             float scale_max = FLT_MAX,
                             ImVec2 graph_size = ImVec2(0, 0),
                             int stride = sizeof(float)) = 0;
  virtual void PlotHistogram(const char* label,
                             float (*values_getter)(void* data, int idx),
                             void* data,
                             int values_count,
                             int values_offset = 0,
                             const char* overlay_text = NULL,
                             float scale_min = FLT_MAX,
                             float scale_max = FLT_MAX,
                             ImVec2 graph_size = ImVec2(0, 0)) = 0;

  // Widgets: Drags (tip: ctrl+click on a drag box to input text)
  virtual bool DragFloat(const char* label,
                         float* v,
                         float v_speed = 1.0f,
                         float v_min = 0.0f,
                         float v_max = 0.0f,
                         const char* display_format = "%.3f",
                         float power = 1.0f) = 0;
  virtual bool DragFloat2(const char* label,
                          float v[2],
                          float v_speed = 1.0f,
                          float v_min = 0.0f,
                          float v_max = 0.0f,
                          const char* display_format = "%.3f",
                          float power = 1.0f) = 0;
  virtual bool DragFloat3(const char* label,
                          float v[3],
                          float v_speed = 1.0f,
                          float v_min = 0.0f,
                          float v_max = 0.0f,
                          const char* display_format = "%.3f",
                          float power = 1.0f) = 0;
  virtual bool DragFloat4(const char* label,
                          float v[4],
                          float v_speed = 1.0f,
                          float v_min = 0.0f,
                          float v_max = 0.0f,
                          const char* display_format = "%.3f",
                          float power = 1.0f) = 0;
  virtual bool DragFloatRange2(const char* label,
                               float* v_current_min,
                               float* v_current_max,
                               float v_speed = 1.0f,
                               float v_min = 0.0f,
                               float v_max = 0.0f,
                               const char* display_format = "%.3f",
                               const char* display_format_max = NULL,
                               float power = 1.0f) = 0;
  virtual bool DragInt(const char* label,
                       int* v,
                       float v_speed = 1.0f,
                       int v_min = 0,
                       int v_max = 0,
                       const char* display_format = "%.0f") = 0;
  virtual bool DragInt2(const char* label,
                        int v[2],
                        float v_speed = 1.0f,
                        int v_min = 0,
                        int v_max = 0,
                        const char* display_format = "%.0f") = 0;
  virtual bool DragInt3(const char* label,
                        int v[3],
                        float v_speed = 1.0f,
                        int v_min = 0,
                        int v_max = 0,
                        const char* display_format = "%.0f") = 0;
  virtual bool DragInt4(const char* label,
                        int v[4],
                        float v_speed = 1.0f,
                        int v_min = 0,
                        int v_max = 0,
                        const char* display_format = "%.0f") = 0;
  virtual bool DragIntRange2(const char* label,
                             int* v_current_min,
                             int* v_current_max,
                             float v_speed = 1.0f,
                             int v_min = 0,
                             int v_max = 0,
                             const char* display_format = "%.0f",
                             const char* display_format_max = NULL) = 0;

  // Widgets: Input
  virtual bool InputText(const char* label,
                         char* buf,
                         size_t buf_size,
                         ImGuiInputTextFlags flags = 0,
                         ImGuiTextEditCallback callback = NULL,
                         void* user_data = NULL) = 0;
  virtual bool InputTextMultiline(const char* label,
                                  char* buf,
                                  size_t buf_size,
                                  const ImVec2& size = ImVec2(0, 0),
                                  ImGuiInputTextFlags flags = 0,
                                  ImGuiTextEditCallback callback = NULL,
                                  void* user_data = NULL) = 0;
  virtual bool InputFloat(const char* label,
                          float* v,
                          float step = 0.0f,
                          float step_fast = 0.0f,
                          int decimal_precision = -1,
                          ImGuiInputTextFlags extra_flags = 0) = 0;
  virtual bool InputFloat2(const char* label,
                           float v[2],
                           int decimal_precision = -1,
                           ImGuiInputTextFlags extra_flags = 0) = 0;
  virtual bool InputFloat3(const char* label,
                           float v[3],
                           int decimal_precision = -1,
                           ImGuiInputTextFlags extra_flags = 0) = 0;
  virtual bool InputFloat4(const char* label,
                           float v[4],
                           int decimal_precision = -1,
                           ImGuiInputTextFlags extra_flags = 0) = 0;
  virtual bool InputInt(const char* label,
                        int* v,
                        int step = 1,
                        int step_fast = 100,
                        ImGuiInputTextFlags extra_flags = 0) = 0;
  virtual bool InputInt2(const char* label,
                         int v[2],
                         ImGuiInputTextFlags extra_flags = 0) = 0;
  virtual bool InputInt3(const char* label,
                         int v[3],
                         ImGuiInputTextFlags extra_flags = 0) = 0;
  virtual bool InputInt4(const char* label,
                         int v[4],
                         ImGuiInputTextFlags extra_flags = 0) = 0;

  // Widgets: Sliders
  virtual bool SliderFloat(const char* label,
                           float* v,
                           float v_min,
                           float v_max,
                           const char* display_format = "%.3f",
                           float power = 1.0f) = 0;
  virtual bool SliderFloat2(const char* label,
                            float v[2],
                            float v_min,
                            float v_max,
                            const char* display_format = "%.3f",
                            float power = 1.0f) = 0;
  virtual bool SliderFloat3(const char* label,
                            float v[3],
                            float v_min,
                            float v_max,
                            const char* display_format = "%.3f",
                            float power = 1.0f) = 0;
  virtual bool SliderFloat4(const char* label,
                            float v[4],
                            float v_min,
                            float v_max,
                            const char* display_format = "%.3f",
                            float power = 1.0f) = 0;
  virtual bool SliderAngle(const char* label,
                           float* v_rad,
                           float v_degrees_min = -360.0f,
                           float v_degrees_max = +360.0f) = 0;
  virtual bool SliderInt(const char* label,
                         int* v,
                         int v_min,
                         int v_max,
                         const char* display_format = "%.0f") = 0;
  virtual bool SliderInt2(const char* label,
                          int v[2],
                          int v_min,
                          int v_max,
                          const char* display_format = "%.0f") = 0;
  virtual bool SliderInt3(const char* label,
                          int v[3],
                          int v_min,
                          int v_max,
                          const char* display_format = "%.0f") = 0;
  virtual bool SliderInt4(const char* label,
                          int v[4],
                          int v_min,
                          int v_max,
                          const char* display_format = "%.0f") = 0;
  virtual bool VSliderFloat(const char* label,
                            const ImVec2& size,
                            float* v,
                            float v_min,
                            float v_max,
                            const char* display_format = "%.3f",
                            float power = 1.0f) = 0;
  virtual bool VSliderInt(const char* label,
                          const ImVec2& size,
                          int* v,
                          int v_min,
                          int v_max,
                          const char* display_format = "%.0f") = 0;

  // Widgets: Trees
  virtual bool TreeNode(const char* str_label_id) = 0;
  virtual bool TreeNode(const char* str_id, const char* fmt, ...)
    IM_PRINTFARGS(2) = 0;
  virtual bool TreeNode(const void* ptr_id, const char* fmt, ...)
    IM_PRINTFARGS(2) = 0;
  virtual bool TreeNodeV(const char* str_id, const char* fmt, va_list args) = 0;
  virtual bool TreeNodeV(const void* ptr_id, const char* fmt, va_list args) = 0;
  virtual void TreePush(const char* str_id = NULL) = 0;
  virtual void TreePush(const void* ptr_id = NULL) = 0;
  virtual void TreePop() = 0;
  virtual void SetNextTreeNodeOpened(bool opened, ImGuiSetCond cond = 0) = 0;

  // Widgets: Selectable / Lists
  virtual bool Selectable(const char* label,
                          bool selected = false,
                          ImGuiSelectableFlags flags = 0,
                          const ImVec2& size = ImVec2(0, 0)) = 0;
  virtual bool Selectable(const char* label,
                          bool* p_selected,
                          ImGuiSelectableFlags flags = 0,
                          const ImVec2& size = ImVec2(0, 0)) = 0;
  virtual bool ListBox(const char* label,
                       int* current_item,
                       const char** items,
                       int items_count,
                       int height_in_items = -1) = 0;
  virtual bool
    ListBox(const char* label,
            int* current_item,
            bool (*items_getter)(void* data, int idx, const char** out_text),
            void* data,
            int items_count,
            int height_in_items = -1) = 0;
  virtual bool ListBoxHeader(const char* label,
                             const ImVec2& size = ImVec2(0, 0)) = 0;
  virtual bool ListBoxHeader(const char* label,
                             int items_count,
                             int height_in_items = -1) = 0;
  virtual void ListBoxFooter() = 0;

  // Widgets: Value() Helpers
  virtual void Value(const char* prefix, bool b) = 0;
  virtual void Value(const char* prefix, int v) = 0;
  virtual void Value(const char* prefix, unsigned int v) = 0;
  virtual void
    Value(const char* prefix, float v, const char* float_format = NULL) = 0;
#if 0
  virtual void Color(const char* prefix, const ImVec4& v) = 0;
  virtual void Color(const char* prefix, unsigned int v) = 0;
#endif

  // Tooltip
  virtual void SetTooltip(const char* fmt, ...) IM_PRINTFARGS(1) = 0;
  virtual void SetTooltipV(const char* fmt, va_list args) = 0;
  virtual void BeginTooltip() = 0;
  virtual void EndTooltip() = 0;

  // Menus
  virtual bool BeginMainMenuBar() = 0;
  virtual void EndMainMenuBar() = 0;
  virtual bool BeginMenuBar() = 0;
  virtual void EndMenuBar() = 0;
  virtual bool BeginMenu(const char* label, bool enabled = true) = 0;
  virtual void EndMenu() = 0;
  virtual bool MenuItem(const char* label,
                        const char* shortcut = NULL,
                        bool selected = false,
                        bool enabled = true) = 0;
  virtual bool MenuItem(const char* label,
                        const char* shortcut,
                        bool* p_selected,
                        bool enabled = true) = 0;

  // Popup
  virtual void OpenPopup(const char* str_id) = 0;
  virtual bool BeginPopup(const char* str_id) = 0;
  virtual bool BeginPopupModal(const char* name,
                               bool* p_opened = NULL,
                               ImGuiWindowFlags extra_flags = 0) = 0;
  virtual bool BeginPopupContextItem(const char* str_id,
                                     int mouse_button = 1) = 0;
  virtual bool BeginPopupContextWindow(bool also_over_items = true,
                                       const char* str_id = NULL,
                                       int mouse_button = 1) = 0;
  virtual bool BeginPopupContextVoid(const char* str_id = NULL,
                                     int mouse_button = 1) = 0;
  virtual void EndPopup() = 0;
  virtual void CloseCurrentPopup() = 0;

  // Logging
  virtual void LogToTTY(int max_depth = -1) = 0;
  virtual void LogToFile(int max_depth = -1, const char* filename = NULL) = 0;
  virtual void LogToClipboard(int max_depth = -1) = 0;
  virtual void LogFinish() = 0;
  virtual void LogButtons() = 0;
  virtual void LogText(const char* fmt, ...) IM_PRINTFARGS(1) = 0;

  // Utilities
  virtual bool IsItemHovered() = 0;
  virtual bool IsItemHoveredRect() = 0;
  virtual bool IsItemActive() = 0;
  virtual bool IsItemVisible() = 0;
  virtual bool IsAnyItemHovered() = 0;
  virtual bool IsAnyItemActive() = 0;
  virtual ImVec2 GetItemRectMin() = 0;
  virtual ImVec2 GetItemRectMax() = 0;
  virtual ImVec2 GetItemRectSize() = 0;
  virtual bool IsWindowHovered() = 0;
  virtual bool IsWindowFocused() = 0;
  virtual bool IsRootWindowFocused() = 0;
  virtual bool IsRootWindowOrAnyChildFocused() = 0;
  virtual bool IsRectVisible(const ImVec2& size) = 0;
  virtual bool IsPosHoveringAnyWindow(const ImVec2& pos) = 0;
  virtual float GetTime() = 0;
  virtual int GetFrameCount() = 0;
  virtual const char* GetStyleColName(ImGuiCol idx) = 0;
  virtual ImVec2 CalcItemRectClosestPoint(const ImVec2& pos,
                                          bool on_edge = false,
                                          float outward = +0.0f) = 0;
  virtual ImVec2 CalcTextSize(const char* text,
                              const char* text_end = NULL,
                              bool hide_text_after_double_hash = false,
                              float wrap_width = -1.0f) = 0;
  virtual void CalcListClipping(int items_count,
                                float items_height,
                                int* out_items_display_start,
                                int* out_items_display_end) = 0;
  virtual bool BeginChildFrame(ImGuiID id,
                               const ImVec2& size,
                               ImGuiWindowFlags extra_flags = 0) = 0;
  virtual void EndChildFrame() = 0;
  virtual ImVec4 ColorConvertU32ToFloat4(ImU32 in) = 0;
  virtual ImU32 ColorConvertFloat4ToU32(const ImVec4& in) = 0;
  virtual void ColorConvertRGBtoHSV(
    float r, float g, float b, float& out_h, float& out_s, float& out_v) = 0;
  virtual void ColorConvertHSVtoRGB(
    float h, float s, float v, float& out_r, float& out_g, float& out_b) = 0;

  // Inputs
  virtual int GetKeyIndex(ImGuiKey key) = 0;
  virtual bool IsKeyDown(int key_index) = 0;
  virtual bool IsKeyPressed(int key_index, bool repeat = true) = 0;
  virtual bool IsKeyReleased(int key_index) = 0;
  virtual bool IsMouseDown(int button) = 0;
  virtual bool IsMouseClicked(int button, bool repeat = false) = 0;
  virtual bool IsMouseDoubleClicked(int button) = 0;
  virtual bool IsMouseReleased(int button) = 0;
  virtual bool IsMouseHoveringWindow() = 0;
  virtual bool IsMouseHoveringAnyWindow() = 0;
  virtual bool IsMouseHoveringRect(const ImVec2& pos_min,
                                   const ImVec2& pos_max,
                                   bool clip = true) = 0;
  virtual bool IsMouseDragging(int button = 0,
                               float lock_threshold = -1.0f) = 0;
  virtual ImVec2 GetMousePos() = 0;
  virtual ImVec2 GetMousePosOnOpeningCurrentPopup() = 0;
  virtual ImVec2 GetMouseDragDelta(int button = 0,
                                   float lock_threshold = -1.0f) = 0;
  virtual void ResetMouseDragDelta(int button = 0) = 0;
  virtual ImGuiMouseCursor GetMouseCursor() = 0;
  virtual void SetMouseCursor(ImGuiMouseCursor type) = 0;
  virtual void CaptureKeyboardFromApp() = 0;
  virtual void CaptureMouseFromApp() = 0;

  virtual void* MemAlloc(size_t sz) = 0;
  virtual void MemFree(void* ptr) = 0;
  virtual const char* GetClipboardText() = 0;
  virtual void SetClipboardText(const char* text) = 0;

  // Internal state/context access
  virtual const char* GetVersion() = 0;
  virtual void* GetInternalState() = 0;
  virtual size_t GetInternalStateSize() = 0;
  virtual void SetInternalState(void* state, bool construct = false) = 0;

  // TODO: Fix code duplication between Log variants.

  template <typename... Args> void LogFormat(const char* format, Args&&... args)
  {
    std::int32_t const num_char =
      _snprintf(nullptr, 0, format, std::forward<Args>(args)...);
    HADESMEM_DETAIL_ASSERT(num_char > 0);
    if (num_char > 0)
    {
      std::vector<char> trace_buffer(static_cast<std::size_t>(num_char + 1));
      std::int32_t const num_char_actual =
        _snprintf(trace_buffer.data(),
                  static_cast<std::size_t>(num_char),
                  format,
                  std::forward<Args>(args)...);
      HADESMEM_DETAIL_ASSERT(num_char_actual > 0);
      (void)num_char_actual;
      auto const trace_buffer_formatted =
        ::hadesmem::detail::WideCharToMultiByte(trace_buffer.data());
      Log(trace_buffer_formatted);
    }
  }

  template <typename... Args>
  void LogFormat(wchar_t const* format, Args&&... args)
  {
    std::int32_t const num_char =
      _snwprintf(nullptr, 0, format, std::forward<Args>(args)...);
    HADESMEM_DETAIL_ASSERT(num_char > 0);
    if (num_char > 0)
    {
      std::vector<char> trace_buffer(static_cast<std::size_t>(num_char + 1));
      std::int32_t const num_char_actual =
        _snwprintf(trace_buffer.data(),
                   static_cast<std::size_t>(num_char),
                   format,
                   std::forward<Args>(args)...);
      HADESMEM_DETAIL_ASSERT(num_char_actual > 0);
      (void)num_char_actual;
      auto const trace_buffer_formatted =
        ::hadesmem::detail::WideCharToMultiByte(trace_buffer.data());
      Log(trace_buffer_formatted);
    }
  }
};

ImguiInterface& GetImguiInterface() noexcept;

void InitializeImgui();
}
}
