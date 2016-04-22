// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "gui.hpp"

#include <cstddef>
#include <cstdint>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/dump.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

#include <cerberus/imgui.hpp>
#include <cerberus/plugin.hpp>
#include <cerberus/render.hpp>

namespace
{
std::size_t g_on_imgui_frame_id = static_cast<std::size_t>(-1);

void OnImguiFrame(hadesmem::cerberus::PluginInterface* cerberus)
{
  auto const& imgui = cerberus->GetImguiInterface();

  imgui->SetNextWindowSize(ImVec2(320, 250), ImGuiSetCond_FirstUseEver);
  imgui->SetNextWindowPos(ImVec2(350, 15), ImGuiSetCond_FirstUseEver);
  if (imgui->Begin("CXExample"))
  {
    imgui->Text("PID: [%lu]", ::GetCurrentProcessId());
  }
  imgui->End();
}
}

void InitializeGui(hadesmem::cerberus::PluginInterface* cerberus)
{
  g_on_imgui_frame_id = cerberus->GetImguiInterface()->RegisterOnFrame(
    [=]() { OnImguiFrame(cerberus); });
}

void CleanupGui(hadesmem::cerberus::PluginInterface* cerberus)
{
  cerberus->GetImguiInterface()->UnregisterOnFrame(g_on_imgui_frame_id);
}
