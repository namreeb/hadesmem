// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

// Modified version of ImGui example and test code.

// TODO: Move our Cerberus GUI and scripting stuff into a lib if possible so we
// can test it here easily. Alternatively, we could just turn this into a basic
// D3D app which we can inject into and use for testing that way.

// TODO: Support other renderers (DX9, DX10, DX12, OpenGL).

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
  wc.lpszClassName = L"ImGui Sandbox";
  ::RegisterClassExW(&wc);
  HWND hwnd = ::CreateWindowExW(0,
                                L"ImGui Sandbox",
                                L"ImGui DirectX11 Sandbox",
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
    ::UnregisterClassW(L"ImGui Sandbox", wc.hInstance);
    return 1;
  }

  ::ShowWindow(hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow(hwnd);

  ImGui_ImplDX11_Init(hwnd, g_device, g_device_context);

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
    if (ImGui::Begin("Sandbox"))
    {
      ImGui::Text("Test");
      ImGui::Text("FPS: %.1f.", ImGui::GetIO().Framerate);
    }
    ImGui::End();

    ImVec4 clear_col = ImColor(114, 144, 154);
    g_device_context->ClearRenderTargetView(g_main_rtv, &clear_col.x);
    ImGui::Render();
    g_swap_chain->Present(0, 0);
  }

  ImGui_ImplDX11_Shutdown();
  CleanupDeviceD3D();
  ::UnregisterClassW(L"ImGui Sandbox", wc.hInstance);

  return 0;
}
