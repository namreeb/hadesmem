// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "gwen.hpp"

#include <hadesmem/config.hpp>

// GWEN is currently only supported under MSVC and Intel C++ due to use of the
// D3DX APIs.

#if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <gwen/gwen.h>
#include <gwen/skins/simple.h>
#include <gwen/skins/texturedbase.h>
#include <gwen/unittest/unittest.h>
#include <gwen/input/windows.h>
#include <gwen/renderers/directx11.h>
#include <gwen/renderers/directx10.h>
#include <gwen/renderers/directx9.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#endif // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)

#include <hadesmem/detail/smart_handle.hpp>

#include "callbacks.hpp"
#include "cursor.hpp"
#include "hook_disabler.hpp"
#include "input.hpp"
#include "plugin.hpp"
#include "render.hpp"
#include "window.hpp"

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnGwenInitializeCallback>&
  GetOnGwenInitializeCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnGwenInitializeCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnGwenCleanupCallback>&
  GetOnGwenCleanupCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnGwenCleanupCallback> callbacks;
  return callbacks;
}

bool& GetGwenInitialized(hadesmem::cerberus::RenderApi api)
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

#if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)

void SetGwenInitialized(hadesmem::cerberus::RenderApi api, bool value)
{
  auto& initialized = GetGwenInitialized(api);
  initialized = value;
}

#endif // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)

bool GwenInitializedAny() HADESMEM_DETAIL_NOEXCEPT
{
  return GetGwenInitialized(hadesmem::cerberus::RenderApi::kD3D9) ||
         GetGwenInitialized(hadesmem::cerberus::RenderApi::kD3D10) ||
         GetGwenInitialized(hadesmem::cerberus::RenderApi::kD3D11) ||
         GetGwenInitialized(hadesmem::cerberus::RenderApi::kOpenGL32);
}

class GwenImpl : public hadesmem::cerberus::GwenInterface
{
public:
  virtual std::size_t RegisterOnInitialize(
    std::function<hadesmem::cerberus::OnGwenInitializeCallback> const& callback)
    final
  {
    auto& callbacks = GetOnGwenInitializeCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnInitialize(std::size_t id) final
  {
    auto& callbacks = GetOnGwenInitializeCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnCleanup(
    std::function<hadesmem::cerberus::OnGwenCleanupCallback> const& callback)
    final
  {
    auto& callbacks = GetOnGwenCleanupCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnCleanup(std::size_t id) final
  {
    auto& callbacks = GetOnGwenCleanupCallbacks();
    return callbacks.Unregister(id);
  }

  virtual bool IsInitialized() final
  {
    return GwenInitializedAny();
  }
};

#if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)

std::unique_ptr<Gwen::Renderer::Base>& GetGwenRenderer()
{
  static std::unique_ptr<Gwen::Renderer::Base> renderer;
  return renderer;
}

std::unique_ptr<Gwen::Skin::TexturedBase>& GetGwenSkin()
{
  static std::unique_ptr<Gwen::Skin::TexturedBase> skin;
  return skin;
}

std::unique_ptr<Gwen::Controls::Canvas>& GetGwenCanvas()
{
  static std::unique_ptr<Gwen::Controls::Canvas> canvas;
  return canvas;
}

std::unique_ptr<UnitTest>& GetGwenUnitTest()
{
  static std::unique_ptr<UnitTest> unit_test;
  return unit_test;
}

std::unique_ptr<Gwen::Input::Windows>& GetGwenInput()
{
  static std::unique_ptr<Gwen::Input::Windows> input;
  return input;
}

// Intentionally leaking to avoid destructors being called in the case that the
// process exits, otherwise if the DirectX DLLs (or OpenGL DLLs etc) are
// unloaded before we are then we would cause an access violation.
struct LeakGwenData
{
public:
  LeakGwenData()
  {
  }

  LeakGwenData(LeakGwenData const&) = delete;

  LeakGwenData& operator=(LeakGwenData const&) = delete;

  ~LeakGwenData()
  {
    GetGwenRenderer().release();
    GetGwenSkin().release();
    GetGwenCanvas().release();
    GetGwenUnitTest().release();
    GetGwenInput().release();
  }
};

LeakGwenData& GetLeakGwenData()
{
  static LeakGwenData leak_gwen_data;
  return leak_gwen_data;
}

void OnInitializeGwenGui(hadesmem::cerberus::RenderApi api, void* device)
{
  if (GwenInitializedAny())
  {
    HADESMEM_DETAIL_TRACE_A("WARNING! GWEN is already initialized. Skipping.");
    return;
  }

  HADESMEM_DETAIL_TRACE_A("Initializing AntTweakBar.");

  auto& renderer = GetGwenRenderer();
  switch (api)
  {
  case hadesmem::cerberus::RenderApi::kD3D9:
    renderer.reset(
      new Gwen::Renderer::DirectX9(static_cast<IDirect3DDevice9*>(device)));
    break;
  case hadesmem::cerberus::RenderApi::kD3D10:
    renderer.reset(
      new Gwen::Renderer::DirectX10(static_cast<ID3D10Device*>(device)));
    break;
  case hadesmem::cerberus::RenderApi::kD3D11:
    renderer.reset(
      new Gwen::Renderer::DirectX11(static_cast<ID3D11Device*>(device)));
    break;
  case hadesmem::cerberus::RenderApi::kOpenGL32:
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{
        "OpenGL currently unsupported for GWEN."});
  default:
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
  }

  SetGwenInitialized(api, true);

  RECT wnd_rect{0, 0, 800, 600};
  if (auto const window = hadesmem::cerberus::GetCurrentWindow())
  {
    HADESMEM_DETAIL_TRACE_A("Have a window.");

    if (!::GetClientRect(window, &wnd_rect) || wnd_rect.right == 0 ||
        wnd_rect.bottom == 0)
    {
      HADESMEM_DETAIL_TRACE_A(
        "GetClientRect failed (or returned an invalid box).");

      wnd_rect = RECT{0, 0, 800, 600};
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Got client rect.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Do not have a window.");
  }
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Window size is %ldx%ld.", wnd_rect.right, wnd_rect.bottom);

  auto& skin = GetGwenSkin();
  skin.reset(new Gwen::Skin::TexturedBase(&*renderer));
  skin->Init("DefaultSkin.png");

  auto& canvas = GetGwenCanvas();
  canvas.reset(new Gwen::Controls::Canvas(&*skin));
  canvas->SetSize(wnd_rect.right, wnd_rect.bottom);
  canvas->SetDrawBackground(false);

  auto& unit = GetGwenUnitTest();
  unit.reset(new UnitTest(&*canvas));
  unit->SetPos(10, 10);

  auto& input = GetGwenInput();
  input.reset(new Gwen::Input::Windows());
  input->Initialize(&*canvas);

  auto& leak = GetLeakGwenData();
  (void)leak;

  HADESMEM_DETAIL_TRACE_A("Calling GWEN initialization callbacks.");

  auto& callbacks = GetOnGwenInitializeCallbacks();
  callbacks.Run(&hadesmem::cerberus::GetGwenInterface());
}

void OnCleanupGwenGui(hadesmem::cerberus::RenderApi api)
{
  if (!GetGwenInitialized(api))
  {
    return;
  }

  HADESMEM_DETAIL_TRACE_A("Calling GWEN cleanup callbacks.");

  auto& callbacks = GetOnGwenCleanupCallbacks();
  callbacks.Run(&hadesmem::cerberus::GetGwenInterface());

  HADESMEM_DETAIL_TRACE_A("Cleaning up GWEN.");

  auto& renderer = GetGwenRenderer();
  renderer.reset();

  auto& skin = GetGwenSkin();
  skin.reset();

  auto& canvas = GetGwenCanvas();
  canvas.reset();

  auto& unit = GetGwenUnitTest();
  unit.reset();

  auto& input = GetGwenInput();
  input.reset();

  SetGwenInitialized(api, false);
}

bool& GetAllGwenVisibility() HADESMEM_DETAIL_NOEXCEPT
{
  static bool visible{false};
  return visible;
}

void SetAllGwenVisibility(bool visible, bool /*old_visible*/)
{
  // Canvas::Show/Canvas::Hide seems to be buggy.
  GetAllGwenVisibility() = visible;
}

void HandleInputQueueEntry(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (!GwenInitializedAny())
  {
    return;
  }

  if (!GetAllGwenVisibility())
  {
    return;
  }

  hadesmem::cerberus::HookDisabler disable_set_cursor_hook{
    &hadesmem::cerberus::GetDisableSetCursorHook()};

  // GWEN doesn't use the entire structure, it only needs what we've
  // initialized.
  MSG message{};
  message.hwnd = hwnd;
  message.message = msg;
  message.wParam = wparam;
  message.lParam = lparam;

  auto& input = GetGwenInput();
  input->ProcessMessage(message);
}

void OnFrameGwen(hadesmem::cerberus::RenderApi /*api*/, void* /*device*/)
{
  if (!GwenInitializedAny())
  {
    return;
  }

  if (!GetAllGwenVisibility())
  {
    return;
  }

  auto& canvas = GetGwenCanvas();
  canvas->RenderCanvas();
}

void OnUnloadPlugins()
{
  SetGwenInitialized(hadesmem::cerberus::RenderApi::kD3D9, false);
  SetGwenInitialized(hadesmem::cerberus::RenderApi::kD3D10, false);
  SetGwenInitialized(hadesmem::cerberus::RenderApi::kD3D11, false);
  SetGwenInitialized(hadesmem::cerberus::RenderApi::kOpenGL32, false);
}

#endif // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
}

namespace hadesmem
{
namespace cerberus
{
GwenInterface& GetGwenInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static GwenImpl gwen;
  return gwen;
}

void InitializeGwen()
{
#if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  auto& input = GetInputInterface();
  input.RegisterOnInputQueueEntry(HandleInputQueueEntry);

  auto& render = GetRenderInterface();
  render.RegisterOnFrame(OnFrameGwen);
  render.RegisterOnInitializeGui(OnInitializeGwenGui);
  render.RegisterOnCleanupGui(OnCleanupGwenGui);
  render.RegisterOnSetGuiVisibility(SetAllGwenVisibility);

  RegisterOnUnloadPlugins(OnUnloadPlugins);
#endif // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
}
}
}
