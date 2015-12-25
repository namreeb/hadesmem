// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

// Modified version of ImGui example and test code.

#include <cctype>
#include <functional>

#include <d3d11.h>
#include <d3dcompiler.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <chaiscript/chaiscript.hpp>
#include <chaiscript/chaiscript_stdlib.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <imgui/imgui.h>
#include <imgui/examples/directx9_example/imgui_impl_dx9.h>
#include <imgui/examples/directx10_example/imgui_impl_dx10.h>
#include <imgui/examples/directx11_example/imgui_impl_dx11.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/dump.hpp>

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd,
                                             UINT msg,
                                             WPARAM wParam,
                                             LPARAM lParam);

namespace
{
using OnFrameFn = std::function<void()>;

OnFrameFn& GetOnFrameCallback()
{
  static OnFrameFn on_frame;
  return on_frame;
}

void SetOnFrameCallback(OnFrameFn const& fn)
{
  GetOnFrameCallback() = fn;
}

#pragma region

bool Impl_Combo(const std::string& label,
                int& current_item,
                const std::vector<chaiscript::Boxed_Value>& items,
                int height_in_items = -1)
{
  std::vector<std::string> vec_str;
  for (const auto& bv : items)
    vec_str.emplace_back(chaiscript::boxed_cast<std::string>(bv));
  auto getter = [](void* data, int idx, const char** out_text) -> bool
  {
    *out_text = static_cast<std::vector<std::string>*>(data)->at(idx).c_str();
    return !!*out_text;
  };
  return ImGui::Combo(label.c_str(),
                      (int*)&current_item,
                      getter,
                      (void*)&vec_str,
                      (int)items.size(),
                      height_in_items);
}

bool Impl_ColorEdit3(const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items)
{
  if (items.size() == 3)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1]),
                 *chaiscript::boxed_cast<float*>(items[2])};
    bool ret = ImGui::ColorEdit3(label.c_str(), f);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_ColorEdit4(const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     bool show_alpha = true)
{
  if (items.size() == 4)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1]),
                 *chaiscript::boxed_cast<float*>(items[2]),
                 *chaiscript::boxed_cast<float*>(items[3])};
    bool ret = ImGui::ColorEdit4(label.c_str(), f, show_alpha);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_DragFloat2(const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     float v_speed = 1.0f,
                     float v_min = 0.0f,
                     float v_max = 0.0f,
                     const std::string& display_format = "%.3f",
                     float power = 1.0f)
{
  if (items.size() == 2)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1])};
    bool ret = ImGui::DragFloat2(
      label.c_str(), f, v_speed, v_min, v_max, display_format.c_str(), power);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_DragFloat3(const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     float v_speed = 1.0f,
                     float v_min = 0.0f,
                     float v_max = 0.0f,
                     const std::string& display_format = "%.3f",
                     float power = 1.0f)
{
  if (items.size() == 3)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1]),
                 *chaiscript::boxed_cast<float*>(items[2])};
    bool ret = ImGui::DragFloat3(
      label.c_str(), f, v_speed, v_min, v_max, display_format.c_str(), power);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_DragFloat4(const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     float v_speed = 1.0f,
                     float v_min = 0.0f,
                     float v_max = 0.0f,
                     const std::string& display_format = "%.3f",
                     float power = 1.0f)
{
  if (items.size() == 4)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1]),
                 *chaiscript::boxed_cast<float*>(items[2]),
                 *chaiscript::boxed_cast<float*>(items[3])};
    bool ret = ImGui::DragFloat4(
      label.c_str(), f, v_speed, v_min, v_max, display_format.c_str(), power);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_DragInt2(const std::string& label,
                   const std::vector<chaiscript::Boxed_Value>& items,
                   float v_speed = 1.0f,
                   int v_min = 0,
                   int v_max = 0,
                   const std::string& display_format = "%.0f")
{
  if (items.size() == 2)
  {
    int v[] = {*chaiscript::boxed_cast<int*>(items[0]),
               *chaiscript::boxed_cast<int*>(items[1])};
    bool ret = ImGui::DragInt2(
      label.c_str(), v, v_speed, v_min, v_max, display_format.c_str());
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<int*>(items[i]) = v[i];
    return ret;
  }
  return false;
}

bool Impl_DragInt3(const std::string& label,
                   const std::vector<chaiscript::Boxed_Value>& items,
                   float v_speed = 1.0f,
                   int v_min = 0,
                   int v_max = 0,
                   const std::string& display_format = "%.0f")
{
  if (items.size() == 3)
  {
    int v[] = {*chaiscript::boxed_cast<int*>(items[0]),
               *chaiscript::boxed_cast<int*>(items[1]),
               *chaiscript::boxed_cast<int*>(items[2])};
    bool ret = ImGui::DragInt3(
      label.c_str(), v, v_speed, v_min, v_max, display_format.c_str());
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<int*>(items[i]) = v[i];
    return ret;
  }
  return false;
}

bool Impl_DragInt4(const std::string& label,
                   const std::vector<chaiscript::Boxed_Value>& items,
                   float v_speed = 1.0f,
                   int v_min = 0,
                   int v_max = 0,
                   const std::string& display_format = "%.0f")
{
  if (items.size() == 4)
  {
    int v[] = {*chaiscript::boxed_cast<int*>(items[0]),
               *chaiscript::boxed_cast<int*>(items[1]),
               *chaiscript::boxed_cast<int*>(items[2]),
               *chaiscript::boxed_cast<int*>(items[3])};
    bool ret = ImGui::DragInt4(
      label.c_str(), v, v_speed, v_min, v_max, display_format.c_str());
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<int*>(items[i]) = v[i];
    return ret;
  }
  return false;
}

bool Impl_InputText(const std::string& label,
                    std::string& text,
                    std::size_t max_size = 255,
                    ImGuiInputTextFlags flags = 0)
{
  char* buffer = new char[max_size]();
  text.copy(buffer, text.size() < max_size ? text.size() : max_size);
  bool ret = ImGui::InputText(label.c_str(), buffer, max_size, flags);
  text = buffer;
  delete buffer;
  return ret;
}

bool Impl_InputTextMultine(const std::string& label,
                           std::string& text,
                           std::size_t max_size = 255,
                           ImVec2 size = ImVec2(0, 0),
                           ImGuiInputTextFlags flags = 0)
{
  char* buffer = new char[max_size]();
  text.copy(buffer, text.size() < max_size ? text.size() : max_size);
  bool ret =
    ImGui::InputTextMultiline(label.c_str(), buffer, max_size, size, flags);
  text = buffer;
  delete buffer;
  return ret;
}

bool Impl_InputFloat2(const std::string& label,
                      const std::vector<chaiscript::Boxed_Value>& items,
                      int decimal_precision = -1,
                      ImGuiInputTextFlags extra_flags = 0)
{
  if (items.size() == 2)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1])};
    bool ret =
      ImGui::InputFloat2(label.c_str(), f, decimal_precision, extra_flags);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_InputFloat3(const std::string& label,
                      const std::vector<chaiscript::Boxed_Value>& items,
                      int decimal_precision = -1,
                      ImGuiInputTextFlags extra_flags = 0)
{
  if (items.size() == 3)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1]),
                 *chaiscript::boxed_cast<float*>(items[2])};
    bool ret =
      ImGui::InputFloat3(label.c_str(), f, decimal_precision, extra_flags);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_InputFloat4(const std::string& label,
                      const std::vector<chaiscript::Boxed_Value>& items,
                      int decimal_precision = -1,
                      ImGuiInputTextFlags extra_flags = 0)
{
  if (items.size() == 4)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1]),
                 *chaiscript::boxed_cast<float*>(items[2]),
                 *chaiscript::boxed_cast<float*>(items[3])};
    bool ret =
      ImGui::InputFloat4(label.c_str(), f, decimal_precision, extra_flags);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_InputInt2(const std::string& label,
                    const std::vector<chaiscript::Boxed_Value>& items,
                    ImGuiInputTextFlags extra_flags = 0)
{
  if (items.size() == 2)
  {
    int v[] = {*chaiscript::boxed_cast<int*>(items[0]),
               *chaiscript::boxed_cast<int*>(items[1])};
    bool ret = ImGui::InputInt2(label.c_str(), v, extra_flags);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<int*>(items[i]) = v[i];
    return ret;
  }
  return false;
}

bool Impl_InputInt3(const std::string& label,
                    const std::vector<chaiscript::Boxed_Value>& items,
                    ImGuiInputTextFlags extra_flags = 0)
{
  if (items.size() == 3)
  {
    int v[] = {*chaiscript::boxed_cast<int*>(items[0]),
               *chaiscript::boxed_cast<int*>(items[1]),
               *chaiscript::boxed_cast<int*>(items[2])};
    bool ret = ImGui::InputInt3(label.c_str(), v, extra_flags);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<int*>(items[i]) = v[i];
    return ret;
  }
  return false;
}

bool Impl_InputInt4(const std::string& label,
                    const std::vector<chaiscript::Boxed_Value>& items,
                    ImGuiInputTextFlags extra_flags = 0)
{
  if (items.size() == 4)
  {
    int v[] = {*chaiscript::boxed_cast<int*>(items[0]),
               *chaiscript::boxed_cast<int*>(items[1]),
               *chaiscript::boxed_cast<int*>(items[2]),
               *chaiscript::boxed_cast<int*>(items[3])};
    bool ret = ImGui::InputInt4(label.c_str(), v, extra_flags);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<int*>(items[i]) = v[i];
    return ret;
  }
  return false;
}

bool Impl_SliderFloat2(const std::string& label,
                       const std::vector<chaiscript::Boxed_Value>& items,
                       float v_min,
                       float v_max,
                       const std::string& display_format = "%.3f",
                       float power = 1.0f)
{
  if (items.size() == 2)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1])};
    bool ret = ImGui::SliderFloat2(
      label.c_str(), f, v_min, v_max, display_format.c_str(), power);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_SliderFloat3(const std::string& label,
                       const std::vector<chaiscript::Boxed_Value>& items,
                       float v_min,
                       float v_max,
                       const std::string& display_format = "%.3f",
                       float power = 1.0f)
{
  if (items.size() == 3)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1]),
                 *chaiscript::boxed_cast<float*>(items[2])};
    bool ret = ImGui::SliderFloat3(
      label.c_str(), f, v_min, v_max, display_format.c_str(), power);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_SliderFloat4(const std::string& label,
                       const std::vector<chaiscript::Boxed_Value>& items,
                       float v_min,
                       float v_max,
                       const std::string& display_format = "%.3f",
                       float power = 1.0f)
{
  if (items.size() == 4)
  {
    float f[] = {*chaiscript::boxed_cast<float*>(items[0]),
                 *chaiscript::boxed_cast<float*>(items[1]),
                 *chaiscript::boxed_cast<float*>(items[2]),
                 *chaiscript::boxed_cast<float*>(items[3])};
    bool ret = ImGui::SliderFloat4(
      label.c_str(), f, v_min, v_max, display_format.c_str(), power);
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<float*>(items[i]) = f[i];
    return ret;
  }
  return false;
}

bool Impl_SliderInt2(const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int v_min,
                     int v_max,
                     const std::string& display_format = "%.0f")
{
  if (items.size() == 2)
  {
    int v[] = {*chaiscript::boxed_cast<int*>(items[0]),
               *chaiscript::boxed_cast<int*>(items[1])};
    bool ret =
      ImGui::SliderInt2(label.c_str(), v, v_min, v_max, display_format.c_str());
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<int*>(items[i]) = v[i];
    return ret;
  }
  return false;
}

bool Impl_SliderInt3(const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int v_min,
                     int v_max,
                     const std::string& display_format = "%.0f")
{
  if (items.size() == 3)
  {
    int v[] = {*chaiscript::boxed_cast<int*>(items[0]),
               *chaiscript::boxed_cast<int*>(items[1]),
               *chaiscript::boxed_cast<int*>(items[2])};
    bool ret =
      ImGui::SliderInt3(label.c_str(), v, v_min, v_max, display_format.c_str());
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<int*>(items[i]) = v[i];
    return ret;
  }
  return false;
}

bool Impl_SliderInt4(const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int v_min,
                     int v_max,
                     const std::string& display_format = "%.0f")
{
  if (items.size() == 4)
  {
    int v[] = {*chaiscript::boxed_cast<int*>(items[0]),
               *chaiscript::boxed_cast<int*>(items[1]),
               *chaiscript::boxed_cast<int*>(items[2]),
               *chaiscript::boxed_cast<int*>(items[3])};
    bool ret =
      ImGui::SliderInt4(label.c_str(), v, v_min, v_max, display_format.c_str());
    for (std::size_t i = 0; i < items.size(); i++)
      *chaiscript::boxed_cast<int*>(items[i]) = v[i];
    return ret;
  }
  return false;
}

bool Impl_ListBox(const std::string& label,
                  int& current_item,
                  const std::vector<chaiscript::Boxed_Value>& items,
                  int height_in_items = -1)
{
  std::vector<std::string> vec_str;
  for (const auto& bv : items)
    vec_str.emplace_back(chaiscript::boxed_cast<std::string>(bv));
  auto getter = [](void* data, int idx, const char** out_text) -> bool
  {
    *out_text = static_cast<std::vector<std::string>*>(data)->at(idx).c_str();
    return !!*out_text;
  };
  return ImGui::ListBox(label.c_str(),
                        (int*)&current_item,
                        getter,
                        (void*)&vec_str,
                        (int)items.size(),
                        height_in_items);
}

#pragma endregion Helper functions

// Credits to JuJu. https://github.com/JuJuBoSc/imgui-chaiscript
chaiscript::ModulePtr GetImGuiChaiScriptModule()
{
  // TODO: Remove this.
  using namespace ImGui;
  using namespace chaiscript;
  using namespace chaiscript::bootstrap;

  ModulePtr mImGui = ModulePtr(new Module());
  ModulePtr mImVec2 = ModulePtr(new Module());
  ModulePtr mImVec4 = ModulePtr(new Module());

#pragma region

  utility::add_class<ImVec2>(
    *mImVec2,
    "ImVec2",
    {constructor<ImVec2()>(), constructor<ImVec2(float x, float y)>()},
    {{fun(&ImVec2::x), "x"}, {fun(&ImVec2::y), "y"}});

  utility::add_class<ImVec4>(
    *mImVec4,
    "ImVec4",
    {constructor<ImVec4()>(),
     constructor<ImVec4(float x, float y, float z, float w)>()},
    {{fun(&ImVec4::x), "x"},
     {fun(&ImVec4::y), "y"},
     {fun(&ImVec4::z), "z"},
     {fun(&ImVec4::w), "w"}});

  mImGui->add(mImVec2);
  mImGui->add(mImVec4);

#pragma endregion ImGui classes

#pragma region

#define IMGUI_REGISTER_CONST(NAME)                                             \
  mImGui->add_global_const(const_var((int)NAME), #NAME);

  IMGUI_REGISTER_CONST(ImGuiWindowFlags_NoTitleBar);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_NoResize);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_NoMove);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_NoScrollbar);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_NoScrollWithMouse);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_AlwaysAutoResize);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_ShowBorders);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_NoSavedSettings);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_NoInputs);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_MenuBar);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_HorizontalScrollbar);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_NoFocusOnAppearing);
  IMGUI_REGISTER_CONST(ImGuiWindowFlags_NoBringToFrontOnFocus);

  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_CharsDecimal);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_CharsHexadecimal);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_CharsUppercase);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_CharsNoBlank);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_AutoSelectAll);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_EnterReturnsTrue);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_CallbackCompletion);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_CallbackHistory);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_CallbackAlways);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_CallbackCharFilter);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_AllowTabInput);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_CtrlEnterForNewLine);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_NoHorizontalScroll);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_AlwaysInsertMode);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_ReadOnly);
  IMGUI_REGISTER_CONST(ImGuiInputTextFlags_Password);

  IMGUI_REGISTER_CONST(ImGuiSelectableFlags_DontClosePopups);
  IMGUI_REGISTER_CONST(ImGuiSelectableFlags_SpanAllColumns);

  IMGUI_REGISTER_CONST(ImGuiKey_Tab);
  IMGUI_REGISTER_CONST(ImGuiKey_LeftArrow);
  IMGUI_REGISTER_CONST(ImGuiKey_RightArrow);
  IMGUI_REGISTER_CONST(ImGuiKey_UpArrow);
  IMGUI_REGISTER_CONST(ImGuiKey_DownArrow);
  IMGUI_REGISTER_CONST(ImGuiKey_PageUp);
  IMGUI_REGISTER_CONST(ImGuiKey_PageDown);
  IMGUI_REGISTER_CONST(ImGuiKey_Home);
  IMGUI_REGISTER_CONST(ImGuiKey_End);
  IMGUI_REGISTER_CONST(ImGuiKey_Delete);
  IMGUI_REGISTER_CONST(ImGuiKey_Backspace);
  IMGUI_REGISTER_CONST(ImGuiKey_Enter);
  IMGUI_REGISTER_CONST(ImGuiKey_Escape);
  IMGUI_REGISTER_CONST(ImGuiKey_A);
  IMGUI_REGISTER_CONST(ImGuiKey_C);
  IMGUI_REGISTER_CONST(ImGuiKey_V);
  IMGUI_REGISTER_CONST(ImGuiKey_X);
  IMGUI_REGISTER_CONST(ImGuiKey_Y);
  IMGUI_REGISTER_CONST(ImGuiKey_Z);

  IMGUI_REGISTER_CONST(ImGuiCol_Text);
  IMGUI_REGISTER_CONST(ImGuiCol_TextDisabled);
  IMGUI_REGISTER_CONST(ImGuiCol_WindowBg);
  IMGUI_REGISTER_CONST(ImGuiCol_ChildWindowBg);
  IMGUI_REGISTER_CONST(ImGuiCol_Border);
  IMGUI_REGISTER_CONST(ImGuiCol_BorderShadow);
  IMGUI_REGISTER_CONST(ImGuiCol_FrameBg);
  IMGUI_REGISTER_CONST(ImGuiCol_FrameBgHovered);
  IMGUI_REGISTER_CONST(ImGuiCol_FrameBgActive);
  IMGUI_REGISTER_CONST(ImGuiCol_TitleBg);
  IMGUI_REGISTER_CONST(ImGuiCol_TitleBgCollapsed);
  IMGUI_REGISTER_CONST(ImGuiCol_TitleBgActive);
  IMGUI_REGISTER_CONST(ImGuiCol_MenuBarBg);
  IMGUI_REGISTER_CONST(ImGuiCol_ScrollbarBg);
  IMGUI_REGISTER_CONST(ImGuiCol_ScrollbarGrab);
  IMGUI_REGISTER_CONST(ImGuiCol_ScrollbarGrabHovered);
  IMGUI_REGISTER_CONST(ImGuiCol_ScrollbarGrabActive);
  IMGUI_REGISTER_CONST(ImGuiCol_ComboBg);
  IMGUI_REGISTER_CONST(ImGuiCol_CheckMark);
  IMGUI_REGISTER_CONST(ImGuiCol_SliderGrab);
  IMGUI_REGISTER_CONST(ImGuiCol_SliderGrabActive);
  IMGUI_REGISTER_CONST(ImGuiCol_Button);
  IMGUI_REGISTER_CONST(ImGuiCol_ButtonHovered);
  IMGUI_REGISTER_CONST(ImGuiCol_ButtonActive);
  IMGUI_REGISTER_CONST(ImGuiCol_Header);
  IMGUI_REGISTER_CONST(ImGuiCol_HeaderHovered);
  IMGUI_REGISTER_CONST(ImGuiCol_HeaderActive);
  IMGUI_REGISTER_CONST(ImGuiCol_Column);
  IMGUI_REGISTER_CONST(ImGuiCol_ColumnHovered);
  IMGUI_REGISTER_CONST(ImGuiCol_ColumnActive);
  IMGUI_REGISTER_CONST(ImGuiCol_ResizeGrip);
  IMGUI_REGISTER_CONST(ImGuiCol_ResizeGripHovered);
  IMGUI_REGISTER_CONST(ImGuiCol_ResizeGripActive);
  IMGUI_REGISTER_CONST(ImGuiCol_CloseButton);
  IMGUI_REGISTER_CONST(ImGuiCol_CloseButtonHovered);
  IMGUI_REGISTER_CONST(ImGuiCol_CloseButtonActive);
  IMGUI_REGISTER_CONST(ImGuiCol_PlotLines);
  IMGUI_REGISTER_CONST(ImGuiCol_PlotLinesHovered);
  IMGUI_REGISTER_CONST(ImGuiCol_PlotHistogram);
  IMGUI_REGISTER_CONST(ImGuiCol_PlotHistogramHovered);
  IMGUI_REGISTER_CONST(ImGuiCol_TextSelectedBg);
  IMGUI_REGISTER_CONST(ImGuiCol_TooltipBg);
  IMGUI_REGISTER_CONST(ImGuiCol_ModalWindowDarkening);

  IMGUI_REGISTER_CONST(ImGuiStyleVar_Alpha);
  IMGUI_REGISTER_CONST(ImGuiStyleVar_WindowPadding);
  IMGUI_REGISTER_CONST(ImGuiStyleVar_WindowRounding);
  IMGUI_REGISTER_CONST(ImGuiStyleVar_WindowMinSize);
  IMGUI_REGISTER_CONST(ImGuiStyleVar_ChildWindowRounding);
  IMGUI_REGISTER_CONST(ImGuiStyleVar_FramePadding);
  IMGUI_REGISTER_CONST(ImGuiStyleVar_FrameRounding);
  IMGUI_REGISTER_CONST(ImGuiStyleVar_ItemSpacing);
  IMGUI_REGISTER_CONST(ImGuiStyleVar_ItemInnerSpacing);
  IMGUI_REGISTER_CONST(ImGuiStyleVar_IndentSpacing);
  IMGUI_REGISTER_CONST(ImGuiStyleVar_GrabMinSize);

  IMGUI_REGISTER_CONST(ImGuiAlign_Left);
  IMGUI_REGISTER_CONST(ImGuiAlign_Center);
  IMGUI_REGISTER_CONST(ImGuiAlign_Right);
  IMGUI_REGISTER_CONST(ImGuiAlign_Top);
  IMGUI_REGISTER_CONST(ImGuiAlign_VCenter);
  IMGUI_REGISTER_CONST(ImGuiAlign_Default);

  IMGUI_REGISTER_CONST(ImGuiMouseCursor_Arrow);
  IMGUI_REGISTER_CONST(ImGuiMouseCursor_TextInput);
  IMGUI_REGISTER_CONST(ImGuiMouseCursor_Move);
  IMGUI_REGISTER_CONST(ImGuiMouseCursor_ResizeNS);
  IMGUI_REGISTER_CONST(ImGuiMouseCursor_ResizeEW);
  IMGUI_REGISTER_CONST(ImGuiMouseCursor_ResizeNESW);
  IMGUI_REGISTER_CONST(ImGuiMouseCursor_ResizeNWSE);

  IMGUI_REGISTER_CONST(ImGuiSetCond_Always);
  IMGUI_REGISTER_CONST(ImGuiSetCond_Once);
  IMGUI_REGISTER_CONST(ImGuiSetCond_FirstUseEver);
  IMGUI_REGISTER_CONST(ImGuiSetCond_Appearing);

#undef IMGUI_REGISTER_CONST

#pragma endregion ImGui enums

#pragma region

  mImGui->add(
    fun([](const std::string& name,
           bool& opened,
           ImVec2 size_on_first_use,
           float bg_alpha,
           ImGuiWindowFlags flags) -> bool
        {
          return Begin(
            name.c_str(), (bool*)&opened, size_on_first_use, bg_alpha, flags);
        }),
    "ImGui_Begin");
  mImGui->add(fun([](const std::string& name,
                     bool& opened,
                     ImVec2 size_on_first_use,
                     float bg_alpha) -> bool
                  {
                    return Begin(name.c_str(),
                                 (bool*)&opened,
                                 size_on_first_use,
                                 bg_alpha);
                  }),
              "ImGui_Begin");
  mImGui->add(
    fun([](const std::string& name, bool& opened, ImVec2 size_on_first_use)
          -> bool
        {
          return Begin(name.c_str(), (bool*)&opened, size_on_first_use);
        }),
    "ImGui_Begin");
  mImGui->add(
    fun(
      [](const std::string& name, bool& opened, ImGuiWindowFlags flags) -> bool
      {
        return Begin(name.c_str(), (bool*)&opened, flags);
      }),
    "ImGui_Begin");
  mImGui->add(fun([](const std::string& name, bool& opened) -> bool
                  {
                    return Begin(name.c_str(), (bool*)&opened);
                  }),
              "ImGui_Begin");
  mImGui->add(fun([](const std::string& name) -> bool
                  {
                    return Begin(name.c_str());
                  }),
              "ImGui_Begin");
  mImGui->add(fun(End), "ImGui_End");
  mImGui->add(fun([](const std::string& name,
                     ImVec2 size,
                     bool border,
                     ImGuiWindowFlags extra_flags) -> bool
                  {
                    return BeginChild(name.c_str(), size, border, extra_flags);
                  }),
              "ImGui_BeginChild");
  mImGui->add(fun([](const std::string& name, ImVec2 size, bool border) -> bool
                  {
                    return BeginChild(name.c_str(), size, border);
                  }),
              "ImGui_BeginChild");
  mImGui->add(fun([](const std::string& name, ImVec2 size) -> bool
                  {
                    return BeginChild(name.c_str(), size);
                  }),
              "ImGui_BeginChild");
  mImGui->add(
    fun([](ImGuiID id, ImVec2 size, bool border, ImGuiWindowFlags extra_flags)
          -> bool
        {
          return BeginChild(id, size, border, extra_flags);
        }),
    "ImGui_BeginChild");
  mImGui->add(fun([](ImGuiID id, ImVec2 size, bool border) -> bool
                  {
                    return BeginChild(id, size, border);
                  }),
              "ImGui_BeginChild");
  mImGui->add(fun([](ImGuiID id, ImVec2 size) -> bool
                  {
                    return BeginChild(id, size);
                  }),
              "ImGui_BeginChild");
  mImGui->add(fun(EndChild), "ImGui_EndChild");
  mImGui->add(fun(GetContentRegionMax), "ImGui_GetContentRegionMax");
  mImGui->add(fun(GetContentRegionAvail), "ImGui_GetContentRegionAvail");
  mImGui->add(fun(GetContentRegionAvailWidth),
              "ImGui_GetContentRegionAvailWidth");
  mImGui->add(fun(GetWindowContentRegionMin),
              "ImGui_GetWindowContentRegionMin");
  mImGui->add(fun(GetWindowContentRegionMax),
              "ImGui_GetWindowContentRegionMax");
  mImGui->add(fun(GetWindowContentRegionWidth),
              "ImGui_GetWindowContentRegionWidth");
  // TODO : GetWindowDrawList
  // TODO : GetWindowFont
  mImGui->add(fun(GetWindowFontSize), "ImGui_GetWindowFontSize");
  mImGui->add(fun(SetWindowFontScale), "ImGui_SetWindowFontScale");
  mImGui->add(fun(GetWindowPos), "ImGui_GetWindowPos");
  mImGui->add(fun(GetWindowSize), "ImGui_GetWindowSize");
  mImGui->add(fun(GetWindowWidth), "ImGui_GetWindowWidth");
  mImGui->add(fun(GetWindowHeight), "ImGui_GetWindowHeight");
  mImGui->add(fun(IsWindowCollapsed), "ImGui_IsWindowCollapsed");

  mImGui->add(fun([](ImVec2 pos, ImGuiSetCond cond) -> void
                  {
                    SetNextWindowPos(pos, cond);
                  }),
              "ImGui_SetNextWindowPos");
  mImGui->add(fun([](ImVec2 pos) -> void
                  {
                    SetNextWindowPos(pos);
                  }),
              "ImGui_SetNextWindowPos");
  mImGui->add(fun(SetNextWindowPosCenter), "ImGui_SetNextWindowPosCenter");
  mImGui->add(fun([](ImVec2 size, ImGuiSetCond cond) -> void
                  {
                    SetNextWindowSize(size, cond);
                  }),
              "ImGui_SetNextWindowSize");
  mImGui->add(fun([](ImVec2 size) -> void
                  {
                    SetNextWindowSize(size);
                  }),
              "ImGui_SetNextWindowSize");
  mImGui->add(fun(SetNextWindowContentSize), "ImGui_SetNextWindowContentSize");
  mImGui->add(fun(SetNextWindowContentWidth),
              "ImGui_SetNextWindowContentWidth");
  mImGui->add(fun([](bool collapsed, ImGuiSetCond cond) -> void
                  {
                    SetNextWindowCollapsed(collapsed, cond);
                  }),
              "ImGui_SetNextWindowCollapsed");
  mImGui->add(fun([](bool collapsed) -> void
                  {
                    SetNextWindowCollapsed(collapsed);
                  }),
              "ImGui_SetNextWindowCollapsed");
  mImGui->add(fun(SetNextWindowFocus), "ImGui_SetNextWindowFocus");
  mImGui->add(fun([](ImVec2 pos, ImGuiSetCond cond) -> void
                  {
                    SetWindowPos(pos, cond);
                  }),
              "ImGui_SetWindowPos");
  mImGui->add(fun([](ImVec2 pos) -> void
                  {
                    SetWindowPos(pos);
                  }),
              "ImGui_SetWindowPos");
  mImGui->add(fun([](ImVec2 size, ImGuiSetCond cond) -> void
                  {
                    SetWindowSize(size, cond);
                  }),
              "ImGui_SetWindowSize");
  mImGui->add(fun([](ImVec2 size) -> void
                  {
                    SetWindowSize(size);
                  }),
              "ImGui_SetWindowSize");
  mImGui->add(fun([](bool collapsed, ImGuiSetCond cond) -> void
                  {
                    SetWindowCollapsed(collapsed, cond);
                  }),
              "ImGui_SetWindowCollapsed");
  mImGui->add(fun([](bool collapsed) -> void
                  {
                    SetWindowCollapsed(collapsed);
                  }),
              "ImGui_SetWindowCollapsed");
  mImGui->add(fun([]() -> void
                  {
                    SetWindowFocus();
                  }),
              "ImGui_SetWindowFocus");
  mImGui->add(
    fun([](const std::string& name, ImVec2 pos, ImGuiSetCond cond) -> void
        {
          SetWindowPos(name.c_str(), pos, cond);
        }),
    "ImGui_SetWindowPos");
  mImGui->add(fun([](const std::string& name, ImVec2 pos) -> void
                  {
                    SetWindowPos(name.c_str(), pos);
                  }),
              "ImGui_SetWindowPos");
  mImGui->add(
    fun([](const std::string& name, ImVec2 size, ImGuiSetCond cond) -> void
        {
          SetWindowSize(name.c_str(), size, cond);
        }),
    "ImGui_SetWindowSize");
  mImGui->add(fun([](const std::string& name, ImVec2 size) -> void
                  {
                    SetWindowSize(name.c_str(), size);
                  }),
              "ImGui_SetWindowSize");
  mImGui->add(
    fun([](const std::string& name, bool collapsed, ImGuiSetCond cond) -> void
        {
          SetWindowCollapsed(name.c_str(), collapsed, cond);
        }),
    "ImGui_SetWindowCollapsed");
  mImGui->add(fun([](const std::string& name, bool collapsed) -> void
                  {
                    SetWindowCollapsed(name.c_str(), collapsed);
                  }),
              "ImGui_SetWindowCollapsed");
  mImGui->add(fun([](const std::string& name) -> void
                  {
                    SetWindowFocus(name.c_str());
                  }),
              "ImGui_SetWindowFocus");

  mImGui->add(fun(GetScrollX), "ImGui_GetScrollX");
  mImGui->add(fun(GetScrollY), "ImGui_GetScrollY");
  mImGui->add(fun(GetScrollMaxX), "ImGui_GetScrollMaxX");
  mImGui->add(fun(GetScrollMaxY), "ImGui_GetScrollMaxY");
  mImGui->add(fun(SetScrollX), "ImGui_SetScrollX");
  mImGui->add(fun(SetScrollY), "ImGui_SetScrollY");
  mImGui->add(fun([](float center_y_ratio) -> void
                  {
                    SetScrollHere(center_y_ratio);
                  }),
              "ImGui_SetScrollHere");
  mImGui->add(fun([]() -> void
                  {
                    SetScrollHere();
                  }),
              "ImGui_SetScrollHere");
  mImGui->add(fun([](float pos_y, float center_y_ratio) -> void
                  {
                    SetScrollFromPosY(pos_y, center_y_ratio);
                  }),
              "ImGui_SetScrollFromPosY");
  mImGui->add(fun([](float pos_y) -> void
                  {
                    SetScrollFromPosY(pos_y);
                  }),
              "ImGui_SetScrollFromPosY");
  mImGui->add(fun([](int offset) -> void
                  {
                    SetKeyboardFocusHere(offset);
                  }),
              "ImGui_SetKeyboardFocusHere");
  mImGui->add(fun([]() -> void
                  {
                    SetKeyboardFocusHere();
                  }),
              "ImGui_SetKeyboardFocusHere");
// TODO : SetStateStorage
// TODO : GetStateStorage

#pragma endregion Window

#pragma region

  // TODO : PushFont
  mImGui->add(fun(PopFont), "ImGui_PopFont");
  mImGui->add(fun([](ImGuiCol idx, ImVec4 col) -> void
                  {
                    PushStyleColor(idx, col);
                  }),
              "ImGui_PushStyleColor");
  mImGui->add(fun([](int count) -> void
                  {
                    PopStyleColor(count);
                  }),
              "ImGui_PopStyleColor");
  mImGui->add(fun([]() -> void
                  {
                    PopStyleColor();
                  }),
              "ImGui_PopStyleColor");
  mImGui->add(fun([](ImGuiStyleVar idx, float val) -> void
                  {
                    PushStyleVar(idx, val);
                  }),
              "ImGui_PushStyleVar");
  mImGui->add(fun([](ImGuiStyleVar idx, ImVec2 val) -> void
                  {
                    PushStyleVar(idx, val);
                  }),
              "ImGui_PushStyleVar");
  mImGui->add(fun([](int count) -> void
                  {
                    PopStyleVar(count);
                  }),
              "ImGui_PopStyleVar");
  mImGui->add(fun([]() -> void
                  {
                    PopStyleVar();
                  }),
              "ImGui_PopStyleVar");

#pragma endregion Parameters stacks(shared)

#pragma region

  mImGui->add(fun(PushItemWidth), "ImGui_PushItemWidth");
  mImGui->add(fun(PopItemWidth), "ImGui_PopItemWidth");
  mImGui->add(fun(CalcItemWidth), "ImGui_CalcItemWidth");
  mImGui->add(fun([](float wrap_pos_x) -> void
                  {
                    PushTextWrapPos(wrap_pos_x);
                  }),
              "ImGui_PushTextWrapPos");
  mImGui->add(fun([]() -> void
                  {
                    PushTextWrapPos();
                  }),
              "ImGui_PushTextWrapPos");
  mImGui->add(fun(PopTextWrapPos), "ImGui_PopTextWrapPos");
  mImGui->add(fun(PushAllowKeyboardFocus), "ImGui_PushAllowKeyboardFocus");
  mImGui->add(fun(PopAllowKeyboardFocus), "ImGui_PopAllowKeyboardFocus");
  mImGui->add(fun(PushButtonRepeat), "ImGui_PushButtonRepeat");
  mImGui->add(fun(PopButtonRepeat), "ImGui_PopButtonRepeat");

#pragma endregion Parameters stacks(current window)

#pragma region

  mImGui->add(fun(BeginGroup), "ImGui_BeginGroup");
  mImGui->add(fun(EndGroup), "ImGui_EndGroup");
  mImGui->add(fun(Separator), "ImGui_Separator");
  mImGui->add(fun([](float local_pos_y, float spacing_w) -> void
                  {
                    SameLine(local_pos_y, spacing_w);
                  }),
              "ImGui_SameLine");
  mImGui->add(fun([](float local_pos_y) -> void
                  {
                    SameLine(local_pos_y);
                  }),
              "ImGui_SameLine");
  mImGui->add(fun([]() -> void
                  {
                    SameLine();
                  }),
              "ImGui_SameLine");
  mImGui->add(fun(Spacing), "ImGui_Spacing");
  mImGui->add(fun([](ImVec2 size) -> void
                  {
                    Dummy(size);
                  }),
              "ImGui_Dummy");
  mImGui->add(fun(Indent), "ImGui_Indent");
  mImGui->add(fun(Unindent), "ImGui_Unindent");
  mImGui->add(fun([](int count, const std::string& id, bool border) -> void
                  {
                    Columns(count, id.c_str(), border);
                  }),
              "ImGui_Columns");
  mImGui->add(fun([](int count, const std::string& id) -> void
                  {
                    Columns(count, id.c_str());
                  }),
              "ImGui_Columns");
  mImGui->add(fun([](int count) -> void
                  {
                    Columns(count);
                  }),
              "ImGui_Columns");
  mImGui->add(fun([]() -> void
                  {
                    Columns();
                  }),
              "ImGui_Columns");
  mImGui->add(fun(NextColumn), "ImGui_NextColumn");
  mImGui->add(fun(GetColumnIndex), "ImGui_GetColumnIndex");
  mImGui->add(fun([](int column_index) -> void
                  {
                    GetColumnOffset(column_index);
                  }),
              "ImGui_GetColumnOffset");
  mImGui->add(fun([]() -> void
                  {
                    GetColumnOffset();
                  }),
              "ImGui_GetColumnOffset");
  mImGui->add(fun(SetColumnOffset), "ImGui_SetColumnOffset");
  mImGui->add(fun([](int column_index) -> void
                  {
                    GetColumnWidth(column_index);
                  }),
              "ImGui_GetColumnWidth");
  mImGui->add(fun([]() -> void
                  {
                    GetColumnWidth();
                  }),
              "ImGui_GetColumnWidth");
  mImGui->add(fun(GetColumnsCount), "ImGui_GetColumnsCount");
  mImGui->add(fun(ImGui::GetCursorPos), "ImGui_GetCursorPos");
  mImGui->add(fun(GetCursorPosX), "ImGui_GetCursorPosX");
  mImGui->add(fun(GetCursorPosY), "ImGui_GetCursorPosY");
  mImGui->add(fun([](ImVec2 local_pos) -> void
                  {
                    ImGui::SetCursorPos(local_pos);
                  }),
              "ImGui_SetCursorPos");
  mImGui->add(fun(SetCursorPosX), "ImGui_SetCursorPosX");
  mImGui->add(fun(SetCursorPosY), "ImGui_SetCursorPosY");
  mImGui->add(fun(GetCursorStartPos), "ImGui_GetCursorStartPos");
  mImGui->add(fun(GetCursorPosY), "ImGui_GetCursorScreenPos");
  mImGui->add(fun([](ImVec2 pos) -> void
                  {
                    ImGui::SetCursorScreenPos(pos);
                  }),
              "ImGui_SetCursorScreenPos");
  mImGui->add(fun(AlignFirstTextHeightToWidgets),
              "ImGui_AlignFirstTextHeightToWidgets");
  mImGui->add(fun(GetTextLineHeight), "ImGui_GetTextLineHeight");
  mImGui->add(fun(GetTextLineHeightWithSpacing),
              "ImGui_GetTextLineHeightWithSpacing");
  mImGui->add(fun(GetCursorPosY), "ImGui_GetItemsLineHeightWithSpacing");

#pragma endregion Cursor / Layout

#pragma region

  mImGui->add(fun([](const std::string& str_id) -> void
                  {
                    PushID(str_id.c_str());
                  }),
              "ImGui_PushID");
  mImGui->add(fun([](const std::string& str_id_begin,
                     const std::string& str_id_end) -> void
                  {
                    PushID(str_id_begin.c_str(), str_id_end.c_str());
                  }),
              "ImGui_PushID");
  mImGui->add(fun([](int int_id) -> void
                  {
                    PushID(int_id);
                  }),
              "ImGui_PushID");
  mImGui->add(fun(PopID), "ImGui_PopID");
  mImGui->add(fun([](const std::string& str_id) -> ImGuiID
                  {
                    return GetID(str_id.c_str());
                  }),
              "ImGui_GetID");
  mImGui->add(fun([](const std::string& str_id_begin,
                     const std::string& str_id_end) -> ImGuiID
                  {
                    return GetID(str_id_begin.c_str(), str_id_end.c_str());
                  }),
              "ImGui_GetID");

#pragma endregion ID scopes

#pragma region

  mImGui->add(fun([](const std::string& text) -> void
                  {
                    Text(text.c_str());
                  }),
              "ImGui_Text");
  mImGui->add(fun([](ImVec4 col, const std::string& text) -> void
                  {
                    TextColored(col, text.c_str());
                  }),
              "ImGui_TextColored");
  mImGui->add(fun([](const std::string& text) -> void
                  {
                    TextDisabled(text.c_str());
                  }),
              "ImGui_TextDisabled");
  mImGui->add(fun([](const std::string& text) -> void
                  {
                    TextWrapped(text.c_str());
                  }),
              "ImGui_TextWrapped");
  mImGui->add(fun([](const std::string& label, const std::string& text) -> void
                  {
                    LabelText(label.c_str(), text.c_str());
                  }),
              "ImGui_TextWrapped");
  mImGui->add(fun(Bullet), "ImGui_Bullet");
  mImGui->add(fun([](const std::string& text) -> void
                  {
                    BulletText(text.c_str());
                  }),
              "ImGui_BulletText");
  mImGui->add(fun([](const std::string& text, ImVec2 size) -> void
                  {
                    Button(text.c_str(), size);
                  }),
              "ImGui_Button");
  mImGui->add(fun([](const std::string& text) -> bool
                  {
                    return Button(text.c_str());
                  }),
              "ImGui_Button");
  mImGui->add(fun([](const std::string& text) -> bool
                  {
                    return SmallButton(text.c_str());
                  }),
              "ImGui_SmallButton");
  mImGui->add(fun([](const std::string& str_id, ImVec2 size) -> void
                  {
                    InvisibleButton(str_id.c_str(), size);
                  }),
              "ImGui_InvisibleButton");
  // TODO : Image
  // TODO : ImageButton
  mImGui->add(fun([](const std::string& label,
                     const std::string& str_id,
                     bool display_frame,
                     bool default_open) -> bool
                  {
                    return CollapsingHeader(label.c_str(),
                                            str_id.c_str(),
                                            display_frame,
                                            default_open);
                  }),
              "ImGui_CollapsingHeader");
  mImGui->add(fun([](const std::string& label,
                     const std::string& str_id,
                     bool display_frame) -> bool
                  {
                    return CollapsingHeader(
                      label.c_str(), str_id.c_str(), display_frame);
                  }),
              "ImGui_CollapsingHeader");
  mImGui->add(
    fun([](const std::string& label, const std::string& str_id) -> bool
        {
          return CollapsingHeader(label.c_str(), str_id.c_str());
        }),
    "ImGui_CollapsingHeader");
  mImGui->add(fun([](const std::string& label) -> bool
                  {
                    return CollapsingHeader(label.c_str());
                  }),
              "ImGui_CollapsingHeader");
  mImGui->add(fun([](const std::string& label, bool& v) -> bool
                  {
                    return Checkbox(label.c_str(), (bool*)&v);
                  }),
              "ImGui_Checkbox");
  mImGui->add(fun([](const std::string& label,
                     unsigned int& flags,
                     unsigned int flags_value) -> bool
                  {
                    return CheckboxFlags(
                      label.c_str(), (unsigned int*)&flags, flags_value);
                  }),
              "ImGui_CheckboxFlags");
  mImGui->add(fun([](const std::string& label, bool active) -> bool
                  {
                    return RadioButton(label.c_str(), active);
                  }),
              "ImGui_RadioButton");
  mImGui->add(fun([](const std::string& label, int& v, int v_button) -> bool
                  {
                    return RadioButton(label.c_str(), (int*)&v, v_button);
                  }),
              "ImGui_RadioButton");
  mImGui->add(fun([](const std::string& label,
                     int& current_item,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int height_in_items) -> bool
                  {
                    return Impl_Combo(
                      label, current_item, items, height_in_items);
                  }),
              "ImGui_Combo");
  mImGui->add(fun([](const std::string& label,
                     int& current_item,
                     const std::vector<chaiscript::Boxed_Value>& items) -> bool
                  {
                    return Impl_Combo(label, current_item, items);
                  }),
              "ImGui_Combo");
  mImGui->add(
    fun([](ImVec4 col, bool smalll_height, bool outline_border) -> bool
        {
          return ColorButton(col, smalll_height, outline_border);
        }),
    "ImGui_ColorButton");
  mImGui->add(fun([](ImVec4 col, bool smalll_height) -> bool
                  {
                    return ColorButton(col, smalll_height);
                  }),
              "ImGui_ColorButton");
  mImGui->add(fun([](ImVec4 col) -> bool
                  {
                    return ColorButton(col);
                  }),
              "ImGui_ColorButton");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items) -> bool
                  {
                    return Impl_ColorEdit3(label, items);
                  }),
              "ImGui_ColorEdit3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     bool show_alpha) -> bool
                  {
                    return Impl_ColorEdit4(label, items, show_alpha);
                  }),
              "ImGui_ColorEdit4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items) -> bool
                  {
                    return Impl_ColorEdit4(label, items);
                  }),
              "ImGui_ColorEdit4");
  mImGui->add(fun(ColorEditMode), "ImGui_ColorEditMode");
// TODO : PlotLines
// TODO : PlotHistogram

#pragma endregion Widgets

#pragma region

  mImGui->add(fun([](const std::string& label,
                     float& v,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format,
                     float power) -> bool
                  {
                    return DragFloat(label.c_str(),
                                     (float*)&v,
                                     v_speed,
                                     v_min,
                                     v_max,
                                     display_format.c_str(),
                                     power);
                  }),
              "ImGui_DragFloat");
  mImGui->add(fun([](const std::string& label,
                     float& v,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format) -> bool
                  {
                    return DragFloat(label.c_str(),
                                     (float*)&v,
                                     v_speed,
                                     v_min,
                                     v_max,
                                     display_format.c_str());
                  }),
              "ImGui_DragFloat");
  mImGui->add(fun([](const std::string& label,
                     float& v,
                     float v_speed,
                     float v_min,
                     float v_max) -> bool
                  {
                    return DragFloat(
                      label.c_str(), (float*)&v, v_speed, v_min, v_max);
                  }),
              "ImGui_DragFloat");
  mImGui->add(
    fun(
      [](const std::string& label, float& v, float v_speed, float v_min) -> bool
      {
        return DragFloat(label.c_str(), (float*)&v, v_speed, v_min);
      }),
    "ImGui_DragFloat");
  mImGui->add(fun([](const std::string& label, float& v, float v_speed) -> bool
                  {
                    return DragFloat(label.c_str(), (float*)&v, v_speed);
                  }),
              "ImGui_DragFloat");
  mImGui->add(fun([](const std::string& label, float& v) -> bool
                  {
                    return DragFloat(label.c_str(), (float*)&v);
                  }),
              "ImGui_DragFloat");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format,
                     float power) -> bool
                  {
                    return Impl_DragFloat2(
                      label, v, v_speed, v_min, v_max, display_format, power);
                  }),
              "ImGui_DragFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_DragFloat2(
                      label, v, v_speed, v_min, v_max, display_format);
                  }),
              "ImGui_DragFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min,
                     float v_max) -> bool
                  {
                    return Impl_DragFloat2(label, v, v_speed, v_min, v_max);
                  }),
              "ImGui_DragFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min) -> bool
                  {
                    return Impl_DragFloat2(label, v, v_speed, v_min);
                  }),
              "ImGui_DragFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed) -> bool
                  {
                    return Impl_DragFloat2(label, v, v_speed);
                  }),
              "ImGui_DragFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v) -> bool
                  {
                    return Impl_DragFloat2(label, v);
                  }),
              "ImGui_DragFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format,
                     float power) -> bool
                  {
                    return Impl_DragFloat3(
                      label, v, v_speed, v_min, v_max, display_format, power);
                  }),
              "ImGui_DragFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_DragFloat3(
                      label, v, v_speed, v_min, v_max, display_format);
                  }),
              "ImGui_DragFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min,
                     float v_max) -> bool
                  {
                    return Impl_DragFloat3(label, v, v_speed, v_min, v_max);
                  }),
              "ImGui_DragFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min) -> bool
                  {
                    return Impl_DragFloat3(label, v, v_speed, v_min);
                  }),
              "ImGui_DragFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed) -> bool
                  {
                    return Impl_DragFloat3(label, v, v_speed);
                  }),
              "ImGui_DragFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v) -> bool
                  {
                    return Impl_DragFloat3(label, v);
                  }),
              "ImGui_DragFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format,
                     float power) -> bool
                  {
                    return Impl_DragFloat4(
                      label, v, v_speed, v_min, v_max, display_format, power);
                  }),
              "ImGui_DragFloat4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_DragFloat4(
                      label, v, v_speed, v_min, v_max, display_format);
                  }),
              "ImGui_DragFloat4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min,
                     float v_max) -> bool
                  {
                    return Impl_DragFloat4(label, v, v_speed, v_min, v_max);
                  }),
              "ImGui_DragFloat4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     float v_min) -> bool
                  {
                    return Impl_DragFloat4(label, v, v_speed, v_min);
                  }),
              "ImGui_DragFloat4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed) -> bool
                  {
                    return Impl_DragFloat4(label, v, v_speed);
                  }),
              "ImGui_DragFloat4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v) -> bool
                  {
                    return Impl_DragFloat4(label, v);
                  }),
              "ImGui_DragFloat4");
  mImGui->add(fun([](const std::string& label,
                     float& v_current_min,
                     float& v_current_max,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format,
                     const std::string& display_format_max,
                     float power) -> bool
                  {
                    return DragFloatRange2(label.c_str(),
                                           (float*)&v_current_min,
                                           (float*)&v_current_max,
                                           v_speed,
                                           v_min,
                                           v_max,
                                           display_format.c_str(),
                                           display_format_max.c_str(),
                                           power);
                  }),
              "ImGui_DragFloatRange2");
  mImGui->add(fun([](const std::string& label,
                     float& v_current_min,
                     float& v_current_max,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format,
                     const std::string& display_format_max) -> bool
                  {
                    return DragFloatRange2(label.c_str(),
                                           (float*)&v_current_min,
                                           (float*)&v_current_max,
                                           v_speed,
                                           v_min,
                                           v_max,
                                           display_format.c_str(),
                                           display_format_max.c_str());
                  }),
              "ImGui_DragFloatRange2");
  mImGui->add(fun([](const std::string& label,
                     float& v_current_min,
                     float& v_current_max,
                     float v_speed,
                     float v_min,
                     float v_max,
                     const std::string& display_format) -> bool
                  {
                    return DragFloatRange2(label.c_str(),
                                           (float*)&v_current_min,
                                           (float*)&v_current_max,
                                           v_speed,
                                           v_min,
                                           v_max,
                                           display_format.c_str());
                  }),
              "ImGui_DragFloatRange2");
  mImGui->add(fun([](const std::string& label,
                     float& v_current_min,
                     float& v_current_max,
                     float v_speed,
                     float v_min,
                     float v_max) -> bool
                  {
                    return DragFloatRange2(label.c_str(),
                                           (float*)&v_current_min,
                                           (float*)&v_current_max,
                                           v_speed,
                                           v_min,
                                           v_max);
                  }),
              "ImGui_DragFloatRange2");
  mImGui->add(fun([](const std::string& label,
                     float& v_current_min,
                     float& v_current_max,
                     float v_speed,
                     float v_min) -> bool
                  {
                    return DragFloatRange2(label.c_str(),
                                           (float*)&v_current_min,
                                           (float*)&v_current_max,
                                           v_speed,
                                           v_min);
                  }),
              "ImGui_DragFloatRange2");
  mImGui->add(fun([](const std::string& label,
                     float& v_current_min,
                     float& v_current_max,
                     float v_speed) -> bool
                  {
                    return DragFloatRange2(label.c_str(),
                                           (float*)&v_current_min,
                                           (float*)&v_current_max,
                                           v_speed);
                  }),
              "ImGui_DragFloatRange2");
  mImGui->add(fun([](const std::string& label,
                     float& v_current_min,
                     float& v_current_max) -> bool
                  {
                    return DragFloatRange2(label.c_str(),
                                           (float*)&v_current_min,
                                           (float*)&v_current_max);
                  }),
              "ImGui_DragFloatRange2");
  mImGui->add(fun([](const std::string& label,
                     int& v,
                     float v_speed,
                     int v_min,
                     int v_max,
                     const std::string& display_format) -> bool
                  {
                    return DragInt(label.c_str(),
                                   (int*)&v,
                                   v_speed,
                                   v_min,
                                   v_max,
                                   display_format.c_str());
                  }),
              "ImGui_DragInt");
  mImGui->add(
    fun(
      [](const std::string& label, int& v, float v_speed, int v_min, int v_max)
        -> bool
      {
        return DragInt(label.c_str(), (int*)&v, v_speed, v_min, v_max);
      }),
    "ImGui_DragInt");
  mImGui->add(
    fun([](const std::string& label, int& v, float v_speed, int v_min) -> bool
        {
          return DragInt(label.c_str(), (int*)&v, v_speed, v_min);
        }),
    "ImGui_DragInt");
  mImGui->add(fun([](const std::string& label, int& v, float v_speed) -> bool
                  {
                    return DragInt(label.c_str(), (int*)&v, v_speed);
                  }),
              "ImGui_DragInt");
  mImGui->add(fun([](const std::string& label, int& v) -> bool
                  {
                    return DragInt(label.c_str(), (int*)&v);
                  }),
              "ImGui_DragInt");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     int v_min,
                     int v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_DragInt2(
                      label, v, v_speed, v_min, v_max, display_format);
                  }),
              "ImGui_DragInt2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     int v_min,
                     int v_max) -> bool
                  {
                    return Impl_DragInt2(label, v, v_speed, v_min, v_max);
                  }),
              "ImGui_DragInt2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     int v_min) -> bool
                  {
                    return Impl_DragInt2(label, v, v_speed, v_min);
                  }),
              "ImGui_DragInt2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed) -> bool
                  {
                    return Impl_DragInt2(label, v, v_speed);
                  }),
              "ImGui_DragInt2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v) -> bool
                  {
                    return Impl_DragInt2(label, v);
                  }),
              "ImGui_DragInt2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     int v_min,
                     int v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_DragInt3(
                      label, v, v_speed, v_min, v_max, display_format);
                  }),
              "ImGui_DragInt3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     int v_min,
                     int v_max) -> bool
                  {
                    return Impl_DragInt3(label, v, v_speed, v_min, v_max);
                  }),
              "ImGui_DragInt3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     int v_min) -> bool
                  {
                    return Impl_DragInt3(label, v, v_speed, v_min);
                  }),
              "ImGui_DragInt3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed) -> bool
                  {
                    return Impl_DragInt3(label, v, v_speed);
                  }),
              "ImGui_DragInt3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v) -> bool
                  {
                    return Impl_DragInt3(label, v);
                  }),
              "ImGui_DragInt3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     int v_min,
                     int v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_DragInt4(
                      label, v, v_speed, v_min, v_max, display_format);
                  }),
              "ImGui_DragInt4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     int v_min,
                     int v_max) -> bool
                  {
                    return Impl_DragInt4(label, v, v_speed, v_min, v_max);
                  }),
              "ImGui_DragInt4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed,
                     int v_min) -> bool
                  {
                    return Impl_DragInt4(label, v, v_speed, v_min);
                  }),
              "ImGui_DragInt4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_speed) -> bool
                  {
                    return Impl_DragInt4(label, v, v_speed);
                  }),
              "ImGui_DragInt4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v) -> bool
                  {
                    return Impl_DragInt4(label, v);
                  }),
              "ImGui_DragInt4");
  mImGui->add(fun([](const std::string& label,
                     int& v_current_min,
                     int& v_current_max,
                     float v_speed,
                     int v_min,
                     int v_max,
                     const std::string& display_format,
                     const std::string& display_format_max) -> bool
                  {
                    return DragIntRange2(label.c_str(),
                                         (int*)&v_current_min,
                                         (int*)&v_current_max,
                                         v_speed,
                                         v_min,
                                         v_max,
                                         display_format.c_str(),
                                         display_format_max.c_str());
                  }),
              "ImGui_DragIntRange2");
  mImGui->add(fun([](const std::string& label,
                     int& v_current_min,
                     int& v_current_max,
                     float v_speed,
                     int v_min,
                     int v_max,
                     const std::string& display_format) -> bool
                  {
                    return DragIntRange2(label.c_str(),
                                         (int*)&v_current_min,
                                         (int*)&v_current_max,
                                         v_speed,
                                         v_min,
                                         v_max,
                                         display_format.c_str());
                  }),
              "ImGui_DragIntRange2");
  mImGui->add(fun([](const std::string& label,
                     int& v_current_min,
                     int& v_current_max,
                     float v_speed,
                     int v_min,
                     int v_max) -> bool
                  {
                    return DragIntRange2(label.c_str(),
                                         (int*)&v_current_min,
                                         (int*)&v_current_max,
                                         v_speed,
                                         v_min,
                                         v_max);
                  }),
              "ImGui_DragIntRange2");
  mImGui->add(fun([](const std::string& label,
                     int& v_current_min,
                     int& v_current_max,
                     float v_speed,
                     int v_min) -> bool
                  {
                    return DragIntRange2(label.c_str(),
                                         (int*)&v_current_min,
                                         (int*)&v_current_max,
                                         v_speed,
                                         v_min);
                  }),
              "ImGui_DragIntRange2");
  mImGui->add(fun([](const std::string& label,
                     int& v_current_min,
                     int& v_current_max,
                     float v_speed) -> bool
                  {
                    return DragIntRange2(label.c_str(),
                                         (int*)&v_current_min,
                                         (int*)&v_current_max,
                                         v_speed);
                  }),
              "ImGui_DragIntRange2");
  mImGui->add(fun([](const std::string& label,
                     int& v_current_min,
                     int& v_current_max) -> bool
                  {
                    return DragIntRange2(label.c_str(),
                                         (int*)&v_current_min,
                                         (int*)&v_current_max);
                  }),
              "ImGui_DragIntRange2");

#pragma endregion Widgets : Drags

#pragma region

  mImGui->add(fun([](const std::string& label,
                     std::string& text,
                     int max_size,
                     ImGuiInputTextFlags flags) -> bool
                  {
                    return Impl_InputText(label, text, max_size, flags);
                  }),
              "ImGui_InputText");
  mImGui->add(
    fun([](const std::string& label, std::string& text, int max_size) -> bool
        {
          return Impl_InputText(label, text, max_size);
        }),
    "ImGui_InputText");
  mImGui->add(fun([](const std::string& label, std::string& text) -> bool
                  {
                    return Impl_InputText(label, text);
                  }),
              "ImGui_InputText");
  mImGui->add(fun([](const std::string& label,
                     std::string& text,
                     int max_size,
                     ImVec2 size,
                     ImGuiInputTextFlags flags) -> bool
                  {
                    return Impl_InputTextMultine(
                      label, text, max_size, size, flags);
                  }),
              "ImGui_InputTextMultiline");
  mImGui->add(fun([](const std::string& label,
                     std::string& text,
                     int max_size,
                     ImVec2 size) -> bool
                  {
                    return Impl_InputTextMultine(label, text, max_size, size);
                  }),
              "ImGui_InputTextMultiline");
  mImGui->add(
    fun([](const std::string& label, std::string& text, int max_size) -> bool
        {
          return Impl_InputTextMultine(label, text, max_size);
        }),
    "ImGui_InputTextMultiline");
  mImGui->add(fun([](const std::string& label, std::string& text) -> bool
                  {
                    return Impl_InputTextMultine(label, text);
                  }),
              "ImGui_InputTextMultiline");
  mImGui->add(fun([](const std::string& label,
                     float& v,
                     float step,
                     float step_fast,
                     int decimal_precision,
                     ImGuiInputTextFlags extra_flags) -> bool
                  {
                    return InputFloat(label.c_str(),
                                      (float*)&v,
                                      step,
                                      step_fast,
                                      decimal_precision,
                                      extra_flags);
                  }),
              "ImGui_InputFloat");
  mImGui->add(
    fun([](const std::string& label,
           float& v,
           float step,
           float step_fast,
           int decimal_precision) -> bool
        {
          return InputFloat(
            label.c_str(), (float*)&v, step, step_fast, decimal_precision);
        }),
    "ImGui_InputFloat");
  mImGui->add(
    fun([](const std::string& label, float& v, float step, float step_fast)
          -> bool
        {
          return InputFloat(label.c_str(), (float*)&v, step, step_fast);
        }),
    "ImGui_InputFloat");
  mImGui->add(fun([](const std::string& label, float& v, float step) -> bool
                  {
                    return InputFloat(label.c_str(), (float*)&v, step);
                  }),
              "ImGui_InputFloat");
  mImGui->add(fun([](const std::string& label, float& v) -> bool
                  {
                    return InputFloat(label.c_str(), (float*)&v);
                  }),
              "ImGui_InputFloat");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int decimal_precision,
                     ImGuiInputTextFlags extra_flags) -> bool
                  {
                    return Impl_InputFloat2(
                      label, items, decimal_precision, extra_flags);
                  }),
              "ImGui_InputFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int decimal_precision) -> bool
                  {
                    return Impl_InputFloat2(label, items, decimal_precision);
                  }),
              "ImGui_InputFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items) -> bool
                  {
                    return Impl_InputFloat2(label, items);
                  }),
              "ImGui_InputFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int decimal_precision,
                     ImGuiInputTextFlags extra_flags) -> bool
                  {
                    return Impl_InputFloat3(
                      label, items, decimal_precision, extra_flags);
                  }),
              "ImGui_InputFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int decimal_precision) -> bool
                  {
                    return Impl_InputFloat3(label, items, decimal_precision);
                  }),
              "ImGui_InputFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items) -> bool
                  {
                    return Impl_InputFloat3(label, items);
                  }),
              "ImGui_InputFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int decimal_precision,
                     ImGuiInputTextFlags extra_flags) -> bool
                  {
                    return Impl_InputFloat4(
                      label, items, decimal_precision, extra_flags);
                  }),
              "ImGui_InputFloat4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int decimal_precision) -> bool
                  {
                    return Impl_InputFloat4(label, items, decimal_precision);
                  }),
              "ImGui_InputFloat4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& items) -> bool
                  {
                    return Impl_InputFloat4(label, items);
                  }),
              "ImGui_InputFloat4");
  mImGui->add(fun([](const std::string& label,
                     int& v,
                     int step,
                     int step_fast,
                     ImGuiInputTextFlags extra_flags) -> bool
                  {
                    return InputInt(
                      label.c_str(), (int*)&v, step, step_fast, extra_flags);
                  }),
              "ImGui_InputInt");
  mImGui->add(
    fun([](const std::string& label, int& v, int step, int step_fast) -> bool
        {
          return InputInt(label.c_str(), (int*)&v, step, step_fast);
        }),
    "ImGui_InputInt");
  mImGui->add(fun([](const std::string& label, int& v, int step) -> bool
                  {
                    return InputInt(label.c_str(), (int*)&v, step);
                  }),
              "ImGui_InputInt");
  mImGui->add(fun([](const std::string& label, int& v) -> bool
                  {
                    return InputInt(label.c_str(), (int*)&v);
                  }),
              "ImGui_InputInt");

#pragma endregion Widgets : Input

#pragma region

  mImGui->add(fun([](const std::string& label,
                     float& v,
                     float v_min,
                     float v_max,
                     const std::string& display_format,
                     float power) -> bool
                  {
                    return SliderFloat(label.c_str(),
                                       (float*)&v,
                                       v_min,
                                       v_max,
                                       display_format.c_str(),
                                       power);
                  }),
              "ImGui_SliderFloat");
  mImGui->add(
    fun([](const std::string& label,
           float& v,
           float v_min,
           float v_max,
           const std::string& display_format) -> bool
        {
          return SliderFloat(
            label.c_str(), (float*)&v, v_min, v_max, display_format.c_str());
        }),
    "ImGui_SliderFloat");
  mImGui->add(
    fun([](const std::string& label, float& v, float v_min, float v_max) -> bool
        {
          return SliderFloat(label.c_str(), (float*)&v, v_min, v_max);
        }),
    "ImGui_SliderFloat");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_min,
                     float v_max,
                     const std::string& display_format,
                     float power) -> bool
                  {
                    return Impl_SliderFloat2(
                      label, v, v_min, v_max, display_format, power);
                  }),
              "ImGui_SliderFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_min,
                     float v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_SliderFloat2(
                      label, v, v_min, v_max, display_format);
                  }),
              "ImGui_SliderFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_min,
                     float v_max) -> bool
                  {
                    return Impl_SliderFloat2(label, v, v_min, v_max);
                  }),
              "ImGui_SliderFloat2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_min,
                     float v_max,
                     const std::string& display_format,
                     float power) -> bool
                  {
                    return Impl_SliderFloat3(
                      label, v, v_min, v_max, display_format, power);
                  }),
              "ImGui_SliderFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_min,
                     float v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_SliderFloat3(
                      label, v, v_min, v_max, display_format);
                  }),
              "ImGui_SliderFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_min,
                     float v_max) -> bool
                  {
                    return Impl_SliderFloat3(label, v, v_min, v_max);
                  }),
              "ImGui_SliderFloat3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_min,
                     float v_max,
                     const std::string& display_format,
                     float power) -> bool
                  {
                    return Impl_SliderFloat4(
                      label, v, v_min, v_max, display_format, power);
                  }),
              "ImGui_SliderFloat4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_min,
                     float v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_SliderFloat4(
                      label, v, v_min, v_max, display_format);
                  }),
              "ImGui_SliderFloat4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     float v_min,
                     float v_max) -> bool
                  {
                    return Impl_SliderFloat4(label, v, v_min, v_max);
                  }),
              "ImGui_SliderFloat4");
  mImGui->add(
    fun([](const std::string& label,
           int& v,
           int v_min,
           int v_max,
           const std::string& display_format) -> bool
        {
          return SliderInt(
            label.c_str(), (int*)&v, v_min, v_max, display_format.c_str());
        }),
    "ImGui_SliderInt");
  mImGui->add(
    fun([](const std::string& label, int& v, int v_min, int v_max) -> bool
        {
          return SliderInt(label.c_str(), (int*)&v, v_min, v_max);
        }),
    "ImGui_SliderInt");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     int v_min,
                     int v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_SliderInt2(
                      label, v, v_min, v_max, display_format);
                  }),
              "ImGui_SliderInt2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     int v_min,
                     int v_max) -> bool
                  {
                    return Impl_SliderInt2(label, v, v_min, v_max);
                  }),
              "ImGui_SliderInt2");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     int v_min,
                     int v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_SliderInt3(
                      label, v, v_min, v_max, display_format);
                  }),
              "ImGui_SliderInt3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     int v_min,
                     int v_max) -> bool
                  {
                    return Impl_SliderInt3(label, v, v_min, v_max);
                  }),
              "ImGui_SliderInt3");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     int v_min,
                     int v_max,
                     const std::string& display_format) -> bool
                  {
                    return Impl_SliderInt4(
                      label, v, v_min, v_max, display_format);
                  }),
              "ImGui_SliderInt4");
  mImGui->add(fun([](const std::string& label,
                     const std::vector<chaiscript::Boxed_Value>& v,
                     int v_min,
                     int v_max) -> bool
                  {
                    return Impl_SliderInt4(label, v, v_min, v_max);
                  }),
              "ImGui_SliderInt4");
  mImGui->add(fun([](const std::string& label,
                     ImVec2 size,
                     float& v,
                     float v_min,
                     float v_max,
                     std::string& display_format,
                     float power) -> bool
                  {
                    return VSliderFloat(label.c_str(),
                                        size,
                                        (float*)&v,
                                        v_min,
                                        v_max,
                                        display_format.c_str(),
                                        power);
                  }),
              "ImGui_VSliderFloat");
  mImGui->add(fun([](const std::string& label,
                     ImVec2 size,
                     float& v,
                     float v_min,
                     float v_max,
                     std::string& display_format) -> bool
                  {
                    return VSliderFloat(label.c_str(),
                                        size,
                                        (float*)&v,
                                        v_min,
                                        v_max,
                                        display_format.c_str());
                  }),
              "ImGui_VSliderFloat");
  mImGui->add(fun([](const std::string& label,
                     ImVec2 size,
                     float& v,
                     float v_min,
                     float v_max) -> bool
                  {
                    return VSliderFloat(
                      label.c_str(), size, (float*)&v, v_min, v_max);
                  }),
              "ImGui_VSliderFloat");
  mImGui->add(fun([](const std::string& label,
                     ImVec2 size,
                     int& v,
                     int v_min,
                     int v_max,
                     std::string& display_format) -> bool
                  {
                    return VSliderInt(label.c_str(),
                                      size,
                                      (int*)&v,
                                      v_min,
                                      v_max,
                                      display_format.c_str());
                  }),
              "ImGui_VSliderInt");
  mImGui->add(
    fun([](const std::string& label, ImVec2 size, int& v, int v_min, int v_max)
          -> bool
        {
          return VSliderInt(label.c_str(), size, (int*)&v, v_min, v_max);
        }),
    "ImGui_VSliderInt");

#pragma endregion Widgets : Sliders

#pragma region

  mImGui->add(fun([](const std::string& str_id) -> bool
                  {
                    return TreeNode(str_id.c_str());
                  }),
              "ImGui_TreeNode");
  mImGui->add(fun([](const std::string& str_id, const std::string& text) -> bool
                  {
                    return TreeNode(str_id.c_str(), text.c_str());
                  }),
              "ImGui_TreeNode");
  mImGui->add(fun([](const std::string& str_id) -> void
                  {
                    TreeNode(str_id.size() ? str_id.c_str() : nullptr);
                  }),
              "ImGui_TreeNodePush");
  mImGui->add(fun([]() -> void
                  {
                    TreePop();
                  }),
              "ImGui_TreePop");
  mImGui->add(fun([](bool opened, ImGuiSetCond cond) -> void
                  {
                    SetNextTreeNodeOpened(opened, cond);
                  }),
              "ImGui_SetNextTreeNodeOpened");
  mImGui->add(fun([](bool opened) -> void
                  {
                    SetNextTreeNodeOpened(opened);
                  }),
              "ImGui_SetNextTreeNodeOpened");

#pragma endregion Widgets : Trees

#pragma region

  mImGui->add(fun([](const std::string& label,
                     bool selected,
                     ImGuiSelectableFlags flags,
                     ImVec2 size) -> bool
                  {
                    return Selectable(label.c_str(), selected, flags, size);
                  }),
              "ImGui_Selectable");
  mImGui->add(fun([](const std::string& label,
                     bool selected,
                     ImGuiSelectableFlags flags) -> bool
                  {
                    return Selectable(label.c_str(), selected, flags);
                  }),
              "ImGui_Selectable");
  mImGui->add(fun([](const std::string& label, bool selected) -> bool
                  {
                    return Selectable(label.c_str(), selected);
                  }),
              "ImGui_Selectable");
  mImGui->add(fun([](const std::string& label) -> bool
                  {
                    return Selectable(label.c_str());
                  }),
              "ImGui_Selectable");
  mImGui->add(fun([](const std::string& label,
                     bool& p_selected,
                     ImGuiSelectableFlags flags,
                     ImVec2 size) -> bool
                  {
                    return Selectable(
                      label.c_str(), (bool*)&p_selected, flags, size);
                  }),
              "ImGui_Selectable");
  mImGui->add(fun([](const std::string& label,
                     bool& p_selected,
                     ImGuiSelectableFlags flags) -> bool
                  {
                    return Selectable(label.c_str(), (bool*)&p_selected, flags);
                  }),
              "ImGui_Selectable");
  mImGui->add(fun([](const std::string& label, bool& p_selected) -> bool
                  {
                    return Selectable(label.c_str(), (bool*)&p_selected);
                  }),
              "ImGui_Selectable");
  mImGui->add(fun([](const std::string& label,
                     int& current_item,
                     const std::vector<chaiscript::Boxed_Value>& items,
                     int height_in_items) -> bool
                  {
                    return Impl_ListBox(
                      label, current_item, items, height_in_items);
                  }),
              "ImGui_ListBox");
  mImGui->add(fun([](const std::string& label,
                     int& current_item,
                     const std::vector<chaiscript::Boxed_Value>& items) -> bool
                  {
                    return Impl_ListBox(label, current_item, items);
                  }),
              "ImGui_ListBox");
  mImGui->add(fun([](const std::string& label, ImVec2 size) -> bool
                  {
                    return ListBoxHeader(label.c_str(), size);
                  }),
              "ImGui_ListBoxHeader");
  mImGui->add(fun([](const std::string& label) -> bool
                  {
                    return ListBoxHeader(label.c_str());
                  }),
              "ImGui_ListBoxHeader");
  mImGui->add(
    fun(
      [](const std::string& label, int items_count, int height_in_items) -> bool
      {
        return ListBoxHeader(label.c_str(), items_count, height_in_items);
      }),
    "ImGui_ListBoxHeader");
  mImGui->add(fun([](const std::string& label, int items_count) -> bool
                  {
                    return ListBoxHeader(label.c_str(), items_count);
                  }),
              "ImGui_ListBoxHeader");
  mImGui->add(fun([]() -> void
                  {
                    ListBoxFooter();
                  }),
              "ImGui_ListBoxFooter");

#pragma endregion Widgets : Selectable / Lists

#pragma region

  mImGui->add(fun([](const std::string& label) -> void
                  {
                    SetTooltip(label.c_str());
                  }),
              "ImGui_SetTooltip");
  mImGui->add(fun(BeginTooltip), "ImGui_BeginTooltip");
  mImGui->add(fun(EndTooltip), "ImGui_EndTooltip");

#pragma endregion Tooltip

#pragma region

  mImGui->add(fun(BeginMainMenuBar), "ImGui_BeginMainMenuBar");
  mImGui->add(fun(EndMainMenuBar), "ImGui_EndMainMenuBar");
  mImGui->add(fun(BeginMenuBar), "ImGui_BeginMenuBar");
  mImGui->add(fun(EndMenuBar), "ImGui_EndMenuBar");
  mImGui->add(fun([](const std::string& label, bool enabled) -> bool
                  {
                    return BeginMenu(label.c_str(), enabled);
                  }),
              "ImGui_BeginMenu");
  mImGui->add(fun([](const std::string& label) -> bool
                  {
                    return BeginMenu(label.c_str());
                  }),
              "ImGui_BeginMenu");
  mImGui->add(fun(ImGui::EndMenu), "ImGui_EndMenu");
  mImGui->add(fun([](const std::string& label,
                     std::string& shortcut,
                     bool selected,
                     bool enabled) -> bool
                  {
                    return MenuItem(
                      label.c_str(), shortcut.c_str(), selected, enabled);
                  }),
              "ImGui_MenuItem");
  mImGui->add(
    fun(
      [](const std::string& label, std::string& shortcut, bool selected) -> bool
      {
        return MenuItem(label.c_str(), shortcut.c_str(), selected);
      }),
    "ImGui_MenuItem");
  mImGui->add(fun([](const std::string& label, std::string& shortcut) -> bool
                  {
                    return MenuItem(label.c_str(), shortcut.c_str());
                  }),
              "ImGui_MenuItem");
  mImGui->add(fun([](const std::string& label) -> bool
                  {
                    return MenuItem(label.c_str());
                  }),
              "ImGui_MenuItem");
  mImGui->add(fun([](const std::string& label,
                     std::string& shortcut,
                     bool& p_selected,
                     bool enabled) -> bool
                  {
                    return MenuItem(label.c_str(),
                                    shortcut.c_str(),
                                    (bool*)&p_selected,
                                    enabled);
                  }),
              "ImGui_MenuItem");
  mImGui->add(
    fun([](const std::string& label, std::string& shortcut, bool& p_selected)
          -> bool
        {
          return MenuItem(label.c_str(), shortcut.c_str(), (bool*)&p_selected);
        }),
    "ImGui_MenuItem");

#pragma endregion Menus

#pragma region

  mImGui->add(fun([](const std::string& str_id) -> void
                  {
                    OpenPopup(str_id.c_str());
                  }),
              "ImGui_OpenPopup");
  mImGui->add(fun([](const std::string& str_id) -> bool
                  {
                    return BeginPopup(str_id.c_str());
                  }),
              "ImGui_BeginPopup");
  mImGui->add(fun([](const std::string& name,
                     bool& p_opened,
                     ImGuiWindowFlags extra_flags) -> bool
                  {
                    return BeginPopupModal(
                      name.c_str(), (bool*)&p_opened, extra_flags);
                  }),
              "ImGui_BeginPopupModal");
  mImGui->add(fun([](const std::string& name, bool& p_opened) -> bool
                  {
                    return BeginPopupModal(name.c_str(), (bool*)&p_opened);
                  }),
              "ImGui_BeginPopupModal");
  mImGui->add(fun([](const std::string& name) -> bool
                  {
                    return BeginPopupModal(name.c_str());
                  }),
              "ImGui_BeginPopupModal");
  mImGui->add(fun([](const std::string& str_id, int mouse_button) -> bool
                  {
                    return BeginPopupContextItem(str_id.c_str(), mouse_button);
                  }),
              "ImGui_BeginPopupContextItem");
  mImGui->add(fun([](const std::string& str_id) -> bool
                  {
                    return BeginPopupContextItem(str_id.c_str());
                  }),
              "ImGui_BeginPopupContextItem");
  mImGui->add(fun([](bool also_over_items,
                     const std::string& str_id,
                     int mouse_button) -> bool
                  {
                    return BeginPopupContextWindow(
                      also_over_items, str_id.c_str(), mouse_button);
                  }),
              "ImGui_BeginPopupContextWindow");
  mImGui->add(fun([](bool also_over_items, const std::string& str_id) -> bool
                  {
                    return BeginPopupContextWindow(also_over_items,
                                                   str_id.c_str());
                  }),
              "ImGui_BeginPopupContextWindow");
  mImGui->add(fun([](bool also_over_items) -> bool
                  {
                    return BeginPopupContextWindow(also_over_items);
                  }),
              "ImGui_BeginPopupContextWindow");
  mImGui->add(fun([]() -> bool
                  {
                    return BeginPopupContextWindow();
                  }),
              "ImGui_BeginPopupContextWindow");
  mImGui->add(fun([](const std::string& str_id, int mouse_button) -> bool
                  {
                    return BeginPopupContextVoid(str_id.c_str(), mouse_button);
                  }),
              "ImGui_BeginPopupContextVoid");
  mImGui->add(fun([](const std::string& str_id) -> bool
                  {
                    return BeginPopupContextVoid(str_id.c_str());
                  }),
              "ImGui_BeginPopupContextVoid");
  mImGui->add(fun([]() -> bool
                  {
                    return BeginPopupContextVoid();
                  }),
              "ImGui_BeginPopupContextVoid");
  mImGui->add(fun(ImGui::EndPopup), "ImGui_EndPopup");
  mImGui->add(fun(ImGui::CloseCurrentPopup), "ImGui_CloseCurrentPopup");

#pragma endregion Popup

#pragma region

  mImGui->add(fun(ImGui::IsItemHovered), "ImGui_IsItemHovered");
  mImGui->add(fun(ImGui::IsItemHoveredRect), "ImGui_IsItemHoveredRect");
  mImGui->add(fun(ImGui::IsItemActive), "ImGui_IsItemActive");
  mImGui->add(fun(ImGui::IsItemVisible), "ImGui_IsItemVisible");
  mImGui->add(fun(ImGui::IsAnyItemHovered), "ImGui_IsAnyItemHovered");
  mImGui->add(fun(ImGui::IsAnyItemActive), "ImGui_IsAnyItemActive");
  mImGui->add(fun(ImGui::GetItemRectMin), "ImGui_GetItemRectMin");
  mImGui->add(fun(ImGui::GetItemRectMax), "ImGui_GetItemRectMax");
  mImGui->add(fun(ImGui::GetItemRectSize), "ImGui_GetItemRectSize");
  mImGui->add(fun(ImGui::IsWindowHovered), "ImGui_IsWindowHovered");
  mImGui->add(fun(ImGui::IsWindowFocused), "ImGui_IsWindowFocused");
  mImGui->add(fun(ImGui::IsRootWindowFocused), "ImGui_IsRootWindowFocused");
  mImGui->add(fun(ImGui::IsRootWindowOrAnyChildFocused),
              "ImGui_IsRootWindowOrAnyChildFocused");
  mImGui->add(fun([](ImVec2 size) -> bool
                  {
                    return IsRectVisible(size);
                  }),
              "ImGui_IsRectVisible");
  mImGui->add(fun([](ImVec2 pos) -> bool
                  {
                    return IsPosHoveringAnyWindow(pos);
                  }),
              "ImGui_IsPosHoveringAnyWindow");
  mImGui->add(fun(ImGui::GetTime), "ImGui_GetTime");
  mImGui->add(fun(ImGui::GetFrameCount), "ImGui_GetFrameCount");
  mImGui->add(fun([](ImGuiCol idx) -> std::string
                  {
                    return std::string(GetStyleColName(idx));
                  }),
              "ImGui_GetStyleColName");
  mImGui->add(fun([](ImVec2 pos, bool on_edge, float outward) -> ImVec2
                  {
                    return CalcItemRectClosestPoint(pos, on_edge, outward);
                  }),
              "ImGui_CalcItemRectClosestPoint");
  mImGui->add(fun([](ImVec2 pos, bool on_edge) -> ImVec2
                  {
                    return CalcItemRectClosestPoint(pos, on_edge);
                  }),
              "ImGui_CalcItemRectClosestPoint");
  mImGui->add(fun([](ImVec2 pos) -> ImVec2
                  {
                    return CalcItemRectClosestPoint(pos);
                  }),
              "ImGui_CalcItemRectClosestPoint");
  mImGui->add(fun([](const std::string& text,
                     const std::string& text_end,
                     bool hide_text_after_double_hash,
                     float wrap_width) -> ImVec2
                  {
                    return CalcTextSize(text.c_str(),
                                        text_end.size() ? text_end.c_str()
                                                        : nullptr,
                                        hide_text_after_double_hash,
                                        wrap_width);
                  }),
              "ImGui_CalcTextSize");
  mImGui->add(fun([](const std::string& text,
                     const std::string& text_end,
                     bool hide_text_after_double_hash) -> ImVec2
                  {
                    return CalcTextSize(text.c_str(),
                                        text_end.size() ? text_end.c_str()
                                                        : nullptr,
                                        hide_text_after_double_hash);
                  }),
              "ImGui_CalcTextSize");
  mImGui->add(
    fun([](const std::string& text, const std::string& text_end) -> ImVec2
        {
          return CalcTextSize(text.c_str(),
                              text_end.size() ? text_end.c_str() : nullptr);
        }),
    "ImGui_CalcTextSize");
  mImGui->add(fun([](const std::string& text) -> ImVec2
                  {
                    return CalcTextSize(text.c_str());
                  }),
              "ImGui_CalcTextSize");
  mImGui->add(fun([](int items_count,
                     float items_height,
                     int& out_items_display_start,
                     int& out_items_display_end) -> void
                  {
                    ImGui::CalcListClipping(items_count,
                                            items_height,
                                            (int*)&out_items_display_start,
                                            (int*)&out_items_display_end);
                  }),
              "ImGui_CalcListClipping");

  mImGui->add(
    fun([](ImGuiID id, ImVec2 size, ImGuiWindowFlags extra_flags) -> bool
        {
          return BeginChildFrame(id, size, extra_flags);
        }),
    "ImGui_BeginChildFrame");
  mImGui->add(fun([](ImGuiID id, ImVec2 size) -> bool
                  {
                    return BeginChildFrame(id, size);
                  }),
              "ImGui_BeginChildFrame");
  mImGui->add(fun(ImGui::EndChildFrame), "ImGui_EndChildFrame");

  mImGui->add(fun(ImGui::ColorConvertU32ToFloat4),
              "ImGui_ColorConvertU32ToFloat4");
  mImGui->add(fun([](ImVec4 in) -> ImU32
                  {
                    return ColorConvertFloat4ToU32(in);
                  }),
              "ImGui_ColorConvertFloat4ToU32");
  mImGui->add(
    fun([](float r, float g, float b, float& out_h, float& out_s, float& out_v)
          -> void
        {
          ColorConvertRGBtoHSV(r, g, b, out_h, out_s, out_v);
        }),
    "ImGui_ColorConvertRGBtoHSV");
  mImGui->add(
    fun([](float h, float s, float v, float& out_r, float& out_g, float& out_b)
          -> void
        {
          ColorConvertHSVtoRGB(h, s, v, out_r, out_g, out_b);
        }),
    "ImGui_ColorConvertHSVtoRGB");

#pragma endregion Utilites

#pragma region

  mImGui->add(fun(ImGui::GetKeyIndex), "ImGui_GetKeyIndex");
  mImGui->add(fun(ImGui::IsKeyDown), "ImGui_IsKeyDown");
  mImGui->add(fun([](int key_index, bool repeat) -> bool
                  {
                    return IsKeyPressed(key_index, repeat);
                  }),
              "ImGui_IsKeyPressed");
  mImGui->add(fun([](int key_index) -> bool
                  {
                    return IsKeyPressed(key_index);
                  }),
              "ImGui_IsKeyPressed");
  mImGui->add(fun(ImGui::IsKeyReleased), "ImGui_IsKeyReleased");
  mImGui->add(fun(ImGui::IsMouseDown), "ImGui_IsMouseDown");
  mImGui->add(fun([](int button, bool repeat) -> bool
                  {
                    return IsMouseClicked(button, repeat);
                  }),
              "ImGui_IsMouseClicked");
  mImGui->add(fun([](int button) -> bool
                  {
                    return IsMouseClicked(button);
                  }),
              "ImGui_IsMouseClicked");
  mImGui->add(fun(ImGui::IsMouseDoubleClicked), "ImGui_IsMouseDoubleClicked");
  mImGui->add(fun(ImGui::IsMouseReleased), "ImGui_IsMouseReleased");
  mImGui->add(fun(ImGui::IsMouseHoveringWindow), "ImGui_IsMouseHoveringWindow");
  mImGui->add(fun(ImGui::IsMouseDoubleClicked),
              "ImGui_IsMouseHoveringAnyWindow");
  mImGui->add(fun(ImGui::IsMouseDoubleClicked), "ImGui_IsMouseDoubleClicked");
  mImGui->add(fun([](ImVec2 pos_min, ImVec2 pos_max, bool clip) -> bool
                  {
                    return IsMouseHoveringRect(pos_min, pos_max, clip);
                  }),
              "ImGui_IsMouseHoveringRect");
  mImGui->add(fun([](ImVec2 pos_min, ImVec2 pos_max) -> bool
                  {
                    return IsMouseHoveringRect(pos_min, pos_max);
                  }),
              "ImGui_IsMouseHoveringRect");
  mImGui->add(fun([](int button, float lock_threshold) -> bool
                  {
                    return IsMouseDragging(button, lock_threshold);
                  }),
              "ImGui_IsMouseDragging");
  mImGui->add(fun([](int button) -> bool
                  {
                    return IsMouseDragging(button);
                  }),
              "ImGui_IsMouseDragging");
  mImGui->add(fun(ImGui::GetMousePos), "ImGui_GetMousePos");
  mImGui->add(fun(ImGui::GetMousePosOnOpeningCurrentPopup),
              "ImGui_GetMousePosOnOpeningCurrentPopup");
  mImGui->add(fun([](int button, float lock_threshold) -> ImVec2
                  {
                    return GetMouseDragDelta(button, lock_threshold);
                  }),
              "ImGui_GetMouseDragDelta");
  mImGui->add(fun([](int button) -> void
                  {
                    ResetMouseDragDelta(button);
                  }),
              "ImGui_ResetMouseDragDelta");
  mImGui->add(fun([]() -> void
                  {
                    return ResetMouseDragDelta();
                  }),
              "ImGui_ResetMouseDragDelta");
  mImGui->add(fun(ImGui::GetMouseCursor), "ImGui_GetMouseCursor");
  mImGui->add(fun(ImGui::SetMouseCursor), "ImGui_SetMouseCursor");
  mImGui->add(fun(ImGui::CaptureKeyboardFromApp), "ImGui_GetMouseCursor");
  mImGui->add(fun(ImGui::CaptureMouseFromApp), "ImGui_CaptureMouseFromApp");

#pragma endregion Inputs

#pragma region

  mImGui->add(fun([]() -> std::string
                  {
                    return std::string(ImGui::GetVersion());
                  }),
              "ImGui_GetVersion");

#pragma endregion Internal state / context

  return mImGui;
}

struct ChaiScriptWrapper
{
  chaiscript::ChaiScript chai_{chaiscript::Std_Lib::library()};

  ChaiScriptWrapper()
  {
    auto const imgui = GetImGuiChaiScriptModule();
    chai_.add(imgui);
    chai_.add(chaiscript::fun(&SetOnFrameCallback), "SetOnFrameCallback");
  }
};

chaiscript::ChaiScript& GetChaiScript()
{
  static ChaiScriptWrapper chai;
  return chai.chai_;
}

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_device_context = nullptr;
IDXGISwapChain* g_swap_chain = nullptr;
ID3D11RenderTargetView* g_main_rtv = nullptr;

void CreateRenderTarget()
{
  DXGI_SWAP_CHAIN_DESC sd;
  g_swap_chain->GetDesc(&sd);

  ID3D11Texture2D* back_buffer;
  D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc = {};
  render_target_view_desc.Format = sd.BufferDesc.Format;
  render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  g_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&back_buffer);
  g_device->CreateRenderTargetView(
    back_buffer, &render_target_view_desc, &g_main_rtv);
  g_device_context->OMSetRenderTargets(1, &g_main_rtv, nullptr);
  back_buffer->Release();
}

void CleanupRenderTarget()
{
  if (g_main_rtv)
  {
    g_main_rtv->Release();
    g_main_rtv = nullptr;
  }
}

HRESULT CreateDeviceD3D(HWND hWnd)
{
  DXGI_SWAP_CHAIN_DESC sd = {};
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  UINT create_device_flags = 0;
#if defined(_DEBUG)
  create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D_FEATURE_LEVEL feature_level;
  const D3D_FEATURE_LEVEL feature_level_array[1] = {
    D3D_FEATURE_LEVEL_11_0,
  };
  auto const create_hr =
    ::D3D11CreateDeviceAndSwapChain(nullptr,
                                    D3D_DRIVER_TYPE_HARDWARE,
                                    nullptr,
                                    create_device_flags,
                                    feature_level_array,
                                    1,
                                    D3D11_SDK_VERSION,
                                    &sd,
                                    &g_swap_chain,
                                    &g_device,
                                    &feature_level,
                                    &g_device_context);
  if (FAILED(create_hr))
  {
    return create_hr;
  }

  D3D11_RASTERIZER_DESC ras_desc = {};
  ras_desc.FillMode = D3D11_FILL_SOLID;
  ras_desc.CullMode = D3D11_CULL_NONE;
  ras_desc.FrontCounterClockwise = FALSE;
  ras_desc.DepthBias = 0;
  ras_desc.SlopeScaledDepthBias = 0.0f;
  ras_desc.DepthBiasClamp = 0;
  ras_desc.DepthClipEnable = TRUE;
  ras_desc.ScissorEnable = TRUE;
  ras_desc.AntialiasedLineEnable = FALSE;
  ras_desc.MultisampleEnable = (sd.SampleDesc.Count > 1) ? TRUE : FALSE;

  ID3D11RasterizerState* ras_state = nullptr;
  g_device->CreateRasterizerState(&ras_desc, &ras_state);
  g_device_context->RSSetState(ras_state);
  ras_state->Release();

  CreateRenderTarget();

  return S_OK;
}

void CleanupDeviceD3D()
{
  CleanupRenderTarget();

  if (g_swap_chain)
  {
    g_swap_chain->Release();
    g_swap_chain = nullptr;
  }

  if (g_device_context)
  {
    g_device_context->Release();
    g_device_context = nullptr;
  }

  if (g_device)
  {
    g_device->Release();
    g_device = nullptr;
  }
}

LRESULT WINAPI WndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (ImGui_ImplDX11_WndProcHandler(wnd, msg, wparam, lparam))
  {
    return true;
  }

  switch (msg)
  {
  case WM_SIZE:
    if (g_device != nullptr && wparam != SIZE_MINIMIZED)
    {
      ImGui_ImplDX11_InvalidateDeviceObjects();
      CleanupRenderTarget();
      g_swap_chain->ResizeBuffers(
        0, LOWORD(lparam), HIWORD(lparam), DXGI_FORMAT_UNKNOWN, 0);
      CreateRenderTarget();
      ImGui_ImplDX11_CreateDeviceObjects();
    }
    return 0;

  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  }

  return ::DefWindowProcW(wnd, msg, wparam, lparam);
}

struct ImGuiLogWindow
{
  ImGuiTextBuffer buf_;
  ImGuiTextFilter filter_;
  std::vector<int> line_offsets_;
  bool scroll_to_bottom_ = false;

  void Clear()
  {
    buf_.clear();
    line_offsets_.clear();
  }

  void AddLog(const char* fmt, ...) IM_PRINTFARGS(2)
  {
    int old_size = buf_.size();
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

  void Draw(const char* title, bool* p_opened = NULL)
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
};

ImGuiLogWindow& GetImGuiLogWindow()
{
  static ImGuiLogWindow log;
  return log;
}


chaiscript::ModulePtr GetCerberusModule()
{
  auto cerberus = chaiscript::ModulePtr(new chaiscript::Module());
  return cerberus;
}

struct ChaiScriptScript
{
  std::unique_ptr<chaiscript::ChaiScript> chai_;

  explicit ChaiScriptScript(std::string const& script)
    : chai_(std::make_unique<chaiscript::ChaiScript>(chaiscript::Std_Lib::library()))
  {
    chai_->add(GetCerberusModule());
    chai_->add(GetImGuiChaiScriptModule());
    chai_->add(chaiscript::fun(&SetOnFrameCallback), "SetOnFrameCallback");

    auto& log = GetImGuiLogWindow();

    try
    {
      auto const val = chai_->eval_file(script);
      if (!val.get_type_info().bare_equal(chaiscript::user_type<void>()))
      {
        try
        {
          auto const str = chai_->eval<
            std::function<std::string(const chaiscript::Boxed_Value& bv) >> (
              "to_string")(val);
          log.AddLog("[Info]: %s\n", str.c_str());
        }
        catch (...)
        {
        }
      }

      chai_->eval("CerberusScriptStart()");
    }
    catch (const chaiscript::exception::eval_error& ee)
    {
      if (ee.call_stack.size() > 0)
      {
        log.AddLog(
          "[Error]: %s during evaluation at (%d,%d)\n",
          boost::current_exception_diagnostic_information().c_str(),
          ee.call_stack[0]->start().line,
          ee.call_stack[0]->start().column);
      }
      else
      {
        log.AddLog(
          "[Error]: %s",
          boost::current_exception_diagnostic_information().c_str());
      }
    }
    catch (...)
    {
      log.AddLog("[Error]: %s",
        boost::current_exception_diagnostic_information().c_str());
    }
  }

  ChaiScriptScript(ChaiScriptScript const&) = delete;

  ChaiScriptScript& operator=(ChaiScriptScript const&) = delete;

  ChaiScriptScript(ChaiScriptScript&&) = default;

  ChaiScriptScript& operator=(ChaiScriptScript&&) = default;

  ~ChaiScriptScript()
  {
    try
    {
      chai_->eval("CerberusScriptStop()");
    }
    catch (...)
    {
    }
  }
};


struct ImGuiConsoleWindow
{
  char input_buf_[256];
  std::vector<std::string> items_;
  bool scroll_to_bottom_;
  std::vector<std::string> history_;
  // -1: new line, 0..history_.size()-1 browsing history.
  int history_pos_;
  std::vector<std::string> commands_;

  ImGuiConsoleWindow()
  {
    ClearLog();
    history_pos_ = -1;
    commands_.push_back("/HELP");
    commands_.push_back("/HISTORY");
    commands_.push_back("/CLEAR");
    commands_.push_back("/TERMINATE");
    commands_.push_back("/LOG");
    // "classify" is here to provide an example of "C"+[tab] completing to "CL"
    // and displaying matches.
    commands_.push_back("/CLASSIFY");
  }

  ~ImGuiConsoleWindow()
  {
    ClearLog();
  }

  void ClearLog()
  {
    items_.clear();
    scroll_to_bottom_ = true;
  }

  void AddLog(const char* fmt, ...) IM_PRINTFARGS(2)
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

  void Draw(const char* title, bool* opened)
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

  static int Stricmp(const char* str1, const char* str2)
  {
    int d;
    while ((d = std::toupper(*str2) - std::toupper(*str1)) == 0 && *str1)
    {
      str1++;
      str2++;
    }
    return d;
  }

  static int Strnicmp(const char* str1, const char* str2, int count)
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

  void ExecCommand(const char* command_line)
  {
    // TODO: Integrate ChaiScript.

    AddLog("# %s\n", command_line);

    // Insert into history. First find match and delete it so it can be pushed
    // to the back. This isn't trying to be smart or optimal.
    // TODO: Use a smarter algorithm.
    history_pos_ = -1;
    for (std::int32_t i = static_cast<std::int32_t>(history_.size()) - 1;
         i >= 0;
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
    else if (Stricmp(command_line, "/LOG") == 0)
    {
      auto& log = GetImGuiLogWindow();
      log.AddLog("[Info] Console command called. Time: %.1f.\n",
                 ImGui::GetTime());
    }
    else
    {
      try
      {
        auto& chai = GetChaiScript();
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

  int TextEditCallback(ImGuiTextEditCallbackData* data)
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
                      (history_pos_ >= 0) ? history_[history_pos_].c_str()
                                          : "");
        data->BufDirty = true;
        data->CursorPos = data->SelectionStart = data->SelectionEnd =
          static_cast<int>(std::strlen(data->Buf));
      }
    }
    }

    return 0;
  }
};

ImGuiConsoleWindow& GetImGuiConsoleWindow()
{
  static ImGuiConsoleWindow console;
  return console;
}
}

int WINAPI wWinMain(HINSTANCE /*instance*/,
                    HINSTANCE /*prev_instance*/,
                    PWSTR /*cmd_line*/,
                    int /*cmd_show*/)
{
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_CLASSDC;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.lpszClassName = L"ImGui Example";
  ::RegisterClassExW(&wc);
  HWND hwnd = ::CreateWindowExW(0,
                                L"ImGui Example",
                                L"ImGui DirectX11 Example",
                                WS_OVERLAPPEDWINDOW,
                                100,
                                100,
                                1280,
                                800,
                                nullptr,
                                nullptr,
                                wc.hInstance,
                                nullptr);

  if (FAILED(CreateDeviceD3D(hwnd)))
  {
    CleanupDeviceD3D();
    ::UnregisterClassW(L"ImGui Example", wc.hInstance);
    return 1;
  }

  ::ShowWindow(hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow(hwnd);

  ImGui_ImplDX11_Init(hwnd, g_device, g_device_context);

  bool show_log_window = false;
  bool show_console_window = false;
  ImVec4 clear_col = ImColor(114, 144, 154);

  MSG msg = {};
  while (msg.message != WM_QUIT)
  {
    if (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
      ::TranslateMessage(&msg);
      ::DispatchMessageW(&msg);
      continue;
    }

    ImGui_ImplDX11_NewFrame();

    ImGui::SetNextWindowSize(ImVec2(320, 250), ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(15, 15), ImGuiSetCond_FirstUseEver);
    if (ImGui::Begin("Cerberus"))
    {
      ImGui::Text("Cerberus Version: %s", HADESMEM_VERSION_STRING);
      ImGui::Text("ImGui Version: %s", ImGui::GetVersion());
      ImGui::Text("Renderer: <FIXME>");
      ImGui::Text("Performance: %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate,
                  ImGui::GetIO().Framerate);

      ImGui::Separator();

      auto& log = GetImGuiLogWindow();

      static char plugin_buf[MAX_PATH + 1] = "";
      ImGui::InputText("Plugin", plugin_buf, sizeof(plugin_buf));
      if (ImGui::Button("Load"))
      {
        // TODO: Implement this.
        log.AddLog("[Info]: Loading plugin %s.\n", plugin_buf);
      }

      ImGui::SameLine();
      if (ImGui::Button("Unload"))
      {
        // TODO: Implement this.
        log.AddLog("[Info]: Unloading plugin %s.\n", plugin_buf);
      }

      ImGui::Separator();

      std::map<std::string, ChaiScriptScript> scripts;

      static char script_buf[MAX_PATH + 1] = "";
      ImGui::InputText("Script", script_buf, sizeof(script_buf));
      if (ImGui::Button("Start"))
      {

        if (scripts.find(script_buf) == std::end(scripts))
        {
          log.AddLog("[Info]: Running script %s.\n", script_buf);
          scripts.emplace(script_buf, ChaiScriptScript( script_buf ));
        }
        else
        {
          log.AddLog("[Warning]: Skipping already running script %.\n", script_buf);

        }
      }

      ImGui::SameLine();
      if (ImGui::Button("Stop"))
      {
        // TODO: Implement this.
        log.AddLog("[Info]: Stopping script %s.\n", script_buf);
      }

      ImGui::Separator();

      if (ImGui::Button("Dump PE"))
      {
        try
        {
          log.AddLog("[Info]: Dumping PE files.\n");
          hadesmem::detail::DumpMemory();
        }
        catch (...)
        {
          log.AddLog("[Error]: %s",
                     boost::current_exception_diagnostic_information().c_str());
        }
      }

      ImGui::SameLine();
      if (ImGui::Button("Debug Break"))
      {
        log.AddLog("[Info]: Performing debug break.\n");
        __debugbreak();
      }

      ImGui::SameLine();
      if (ImGui::Button("Eject"))
      {
        // TODO: Implement this. Useful for speeding up unload/reload dev
        // cycles.
        log.AddLog("[Info]: Ejecting.\n");
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
      auto& console = GetImGuiConsoleWindow();
      console.Draw("Console", &show_console_window);
    }

    if (show_log_window)
    {
      auto& log = GetImGuiLogWindow();
      log.Draw("Log", &show_log_window);
    }

    if (auto& callback = GetOnFrameCallback())
    {
      callback();
    }

    g_device_context->ClearRenderTargetView(g_main_rtv, &clear_col.x);
    ImGui::Render();
    g_swap_chain->Present(0, 0);
  }

  ImGui_ImplDX11_Shutdown();
  CleanupDeviceD3D();
  ::UnregisterClassW(L"ImGui Example", wc.hInstance);

  return 0;
}
