// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "imgui.hpp"

#include <windows.h>

#include <d3d11.h>
#include <d3d9.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <imgui/imgui.h>
#include <imgui/examples/directx9_example/imgui_impl_dx9.h>
#include <imgui/examples/directx11_example/imgui_impl_dx11.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include "callbacks.hpp"
#include "cursor.hpp"
#include "hook_disabler.hpp"
#include "input.hpp"
#include "plugin.hpp"
#include "render.hpp"
#include "window.hpp"

extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd,
                                            UINT msg,
                                            WPARAM wParam,
                                            LPARAM lParam);

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd,
                                             UINT msg,
                                             WPARAM wParam,
                                             LPARAM lParam);

// TODO: Fix thread safety of initialization etc.

// TODO: Consolidate code across different GUIs where possible.

// TODO: Consolidate all notes/todos across different GUIs where they are not
// GUI-specific.

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

  virtual bool IsInitialized() final
  {
    return ImguiInitializedAny();
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
    // TODO: Add this. Shouldn't be too hard to port.
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "Currently unsupported render API."});

  case hadesmem::cerberus::RenderApi::kD3D11:
  {
    ID3D11DeviceContext* device_context = nullptr;
    auto const device_ = static_cast<ID3D11Device*>(device);
    device_->GetImmediateContext(&device_context);
    ImGui_ImplDX11_Init(window.GetCurrentWindow(), device_, device_context);
    break;
  }

  case hadesmem::cerberus::RenderApi::kOpenGL32:
    // TODO: Add this. Imgui supports it.
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "Currently unsupported render API."});
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

  case hadesmem::cerberus::RenderApi::kD3D11:
    HADESMEM_DETAIL_TRACE_A("Cleaning up for D3D11.");
    ImGui_ImplDX11_Shutdown();
    break;

  case hadesmem::cerberus::RenderApi::kD3D10:
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

  case hadesmem::cerberus::RenderApi::kD3D11:
    ImGui_ImplDX11_NewFrame();
    break;

  case hadesmem::cerberus::RenderApi::kD3D10:
  case hadesmem::cerberus::RenderApi::kOpenGL32:
    break;

  default:
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
  }

  auto const window_name =
    "Cerberus (" + hadesmem::cerberus::GetRenderApiName(api) + ")";

  ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
  ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_FirstUseEver);
  ImGui::Begin(window_name.c_str());
  ImGui::Text("Hello");
  ImGui::End();

  ImGui::Render();
}

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
  static ImguiImpl ant_tweak_bar;
  return ant_tweak_bar;
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
