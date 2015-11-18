// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "cegui.hpp"

#include <memory>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/smart_handle.hpp>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/Direct3D9/Renderer.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include "callbacks.hpp"
#include "cursor.hpp"
#include "hook_disabler.hpp"
#include "input.hpp"
#include "plugin.hpp"
#include "render.hpp"
#include "window.hpp"

// TODO: Fix this to actually support multiple devices, OpenGL, expose to
// plugins, multiple simultaneous renderers (e.g. D3D10 and D3D11 at the same
// time), multiple windows, etc.

// TODO: Fix to load skins etc from correct full path rather than game dir.

// TODO: Add a basic console.

// TODO: Fix thread safety of initialization etc.

namespace
{
class GameConsoleWindow
{
public:
  GameConsoleWindow();
  void SetVisible(bool visible);
  bool IsVisible();

private:
  void CreateCEGUIWindow();
  void RegisterHandlers();
  bool HandleTextSubmitted(CEGUI::EventArgs const& e);
  bool HandleSendButtonPressed(CEGUI::EventArgs const& e);
  void ParseText(CEGUI::String const& msg);
  void OutputText(CEGUI::String const& msg,
                  CEGUI::Colour colour = CEGUI::Colour(0xFFFFFFFF));

  CEGUI::Window* console_wnd_{nullptr};
};

GameConsoleWindow::GameConsoleWindow()
{
  CreateCEGUIWindow();
  SetVisible(false);
}

void GameConsoleWindow::CreateCEGUIWindow()
{
  CEGUI::WindowManager* pWindowManager =
    CEGUI::WindowManager::getSingletonPtr();
  console_wnd_ = pWindowManager->loadLayoutFromFile("console.layout");
  if (console_wnd_)
  {
    CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(
      console_wnd_);
    RegisterHandlers();
  }
  else
  {
    CEGUI::Logger::getSingleton().logEvent(
      "Error: Unable to load the ConsoleWindow from .layout");
  }
}

void GameConsoleWindow::RegisterHandlers()
{
  console_wnd_->getChild("Submit")
    ->subscribeEvent(CEGUI::PushButton::EventClicked,
                     CEGUI::Event::Subscriber(
                       &GameConsoleWindow::HandleSendButtonPressed, this));

  console_wnd_->getChild("Editbox")->subscribeEvent(
    CEGUI::Editbox::EventTextAccepted,
    CEGUI::Event::Subscriber(&GameConsoleWindow::HandleTextSubmitted, this));
}

bool GameConsoleWindow::HandleTextSubmitted(CEGUI::EventArgs const& e)
{
  CEGUI::String Msg = console_wnd_->getChild("Editbox")->getText();
  ParseText(Msg);
  CEGUI::Window* edit_box = console_wnd_->getChild("Editbox");
  edit_box->setText("");
  edit_box->activate();
  return true;
}

bool GameConsoleWindow::HandleSendButtonPressed(CEGUI::EventArgs const& e)
{
  CEGUI::String const msg = console_wnd_->getChild("Editbox")->getText();
  ParseText(msg);
  CEGUI::Window* edit_box = console_wnd_->getChild("Editbox");
  edit_box->setText("");
  edit_box->activate();

  return true;
}

void GameConsoleWindow::ParseText(CEGUI::String const& msg)
{
  std::string inString = msg.c_str();
  if (inString.length() >= 1)
  {
    if (inString.at(0) == '/')
    {
      std::string::size_type commandEnd = inString.find(" ", 1);
      std::string command = inString.substr(1, commandEnd - 1);
      std::string commandArgs =
        inString.substr(commandEnd + 1, inString.length() - (commandEnd + 1));
      for (std::string::size_type i = 0; i < command.length(); i++)
      {
        command[i] = tolower(command[i]);
      }

      if (command == "say")
      {
        std::string outString = "You:" + inString;
        OutputText(outString);
      }
      else if (command == "quit")
      {
        OutputText("quit command detected");
      }
      else if (command == "help")
      {
        OutputText("help command detected");
      }
      else
      {
        std::string outString = "<" + inString + "> is an invalid command.";
        auto const red = CEGUI::Colour(1.0f, 0.0f, 0.0f);
        OutputText(outString, red);
      }
    }
    else
    {
      OutputText(inString);
    }
  }
}

void GameConsoleWindow::OutputText(CEGUI::String const& msg,
                                   CEGUI::Colour colour)
{
  CEGUI::Listbox* outputWindow =
    static_cast<CEGUI::Listbox*>(console_wnd_->getChild("History"));
  CEGUI::ListboxTextItem* newItem = 0;
  newItem = new CEGUI::ListboxTextItem(msg);
  newItem->setTextColours(colour);
  outputWindow->addItem(newItem);
}

void GameConsoleWindow::SetVisible(bool visible)
{
  console_wnd_->setVisible(visible);

  CEGUI::Editbox* editBox =
    static_cast<CEGUI::Editbox*>(console_wnd_->getChild("Editbox"));
  if (visible)
  {
    editBox->activate();
  }
  else
  {
    editBox->deactivate();
  }
}

bool GameConsoleWindow::IsVisible()
{
  return console_wnd_->isVisible();
}

std::unique_ptr<GameConsoleWindow>& GetGameConsoleWindow()
{
  static std::unique_ptr<GameConsoleWindow> console;
  return console;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnCeguiInitializeCallback>&
  GetOnCeguiInitializeCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnCeguiInitializeCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnCeguiCleanupCallback>&
  GetOnCeguiCleanupCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnCeguiCleanupCallback> callbacks;
  return callbacks;
}

bool& GetCeguiInitialized(hadesmem::cerberus::RenderApi api)
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

void SetCeguiInitialized(hadesmem::cerberus::RenderApi api, bool value)
{
  auto& initialized = GetCeguiInitialized(api);
  initialized = value;
}

bool CeguiInitializedAny() noexcept
{
  return GetCeguiInitialized(hadesmem::cerberus::RenderApi::kD3D9) ||
         GetCeguiInitialized(hadesmem::cerberus::RenderApi::kD3D10) ||
         GetCeguiInitialized(hadesmem::cerberus::RenderApi::kD3D11) ||
         GetCeguiInitialized(hadesmem::cerberus::RenderApi::kOpenGL32);
}

class CeguiImpl : public hadesmem::cerberus::CeguiInterface
{
public:
  virtual std::size_t RegisterOnInitialize(
    std::function<hadesmem::cerberus::OnCeguiInitializeCallback> const&
      callback) final
  {
    auto& callbacks = GetOnCeguiInitializeCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnInitialize(std::size_t id) final
  {
    auto& callbacks = GetOnCeguiInitializeCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnCleanup(
    std::function<hadesmem::cerberus::OnCeguiCleanupCallback> const& callback)
    final
  {
    auto& callbacks = GetOnCeguiCleanupCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnCleanup(std::size_t id) final
  {
    auto& callbacks = GetOnCeguiCleanupCallbacks();
    return callbacks.Unregister(id);
  }

  virtual bool IsInitialized() final
  {
    return CeguiInitializedAny();
  }
};

void OnInitializeCeguiGui(hadesmem::cerberus::RenderApi api, void* device)
{
  if (CeguiInitializedAny())
  {
    HADESMEM_DETAIL_TRACE_A("WARNING! Cegui is already initialized. Skipping.");
    return;
  }

  HADESMEM_DETAIL_TRACE_A("Initializing Cegui.");

  switch (api)
  {
  case hadesmem::cerberus::RenderApi::kD3D9:
    CEGUI::Direct3D9Renderer::bootstrapSystem(
      static_cast<IDirect3DDevice9*>(device));
    break;
  case hadesmem::cerberus::RenderApi::kD3D10:
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "Temporarily unsupported render API."});
    break;
  case hadesmem::cerberus::RenderApi::kD3D11:
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "Temporarily unsupported render API."});
    break;
  case hadesmem::cerberus::RenderApi::kOpenGL32:
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{} << hadesmem::ErrorString{
                                      "Temporarily unsupported render API."});
    break;
  default:
    HADESMEM_DETAIL_ASSERT(false);
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
  }

  SetCeguiInitialized(api, true);

  RECT wnd_rect{0, 0, 800, 600};
  auto& window_interface = hadesmem::cerberus::GetWindowInterface();
  if (auto const window = window_interface.GetCurrentWindow())
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

  CEGUI::DefaultResourceProvider* rp =
    static_cast<CEGUI::DefaultResourceProvider*>(
      CEGUI::System::getSingleton().getResourceProvider());
  auto const cegui_path =
    hadesmem::detail::WideCharToMultiByte(hadesmem::detail::CombinePath(
      hadesmem::detail::GetSelfDirPath(), L"cegui"));
  rp->setResourceGroupDirectory("schemes", cegui_path);
  rp->setResourceGroupDirectory("imagesets", cegui_path);
  rp->setResourceGroupDirectory("fonts", cegui_path);
  rp->setResourceGroupDirectory("layouts", cegui_path);
  rp->setResourceGroupDirectory("looknfeels", cegui_path);
  rp->setResourceGroupDirectory("lua_scripts", cegui_path);

  CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
  CEGUI::Font::setDefaultResourceGroup("fonts");
  CEGUI::Scheme::setDefaultResourceGroup("schemes");
  CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
  CEGUI::WindowManager::setDefaultResourceGroup("layouts");
  CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");

  CEGUI::SchemeManager::getSingletonPtr()->createFromFile("TaharezLook.scheme");
  CEGUI::Font& defaultFont =
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-12.font");
  CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultFont(
    &defaultFont);

  CEGUI::System::getSingleton().notifyDisplaySizeChanged(
    CEGUI::Sizef(wnd_rect.right, wnd_rect.bottom));

  auto& console = GetGameConsoleWindow();
  console = std::make_unique<GameConsoleWindow>();

  HADESMEM_DETAIL_TRACE_A("Calling Cegui initialization callbacks.");

  auto& callbacks = GetOnCeguiInitializeCallbacks();
  callbacks.Run(&hadesmem::cerberus::GetCeguiInterface());
}

void OnCleanupCeguiGui(hadesmem::cerberus::RenderApi api)
{
  if (!GetCeguiInitialized(api))
  {
    return;
  }

  HADESMEM_DETAIL_TRACE_A("Calling Cegui cleanup callbacks.");

  auto& callbacks = GetOnCeguiCleanupCallbacks();
  callbacks.Run(&hadesmem::cerberus::GetCeguiInterface());

  HADESMEM_DETAIL_TRACE_A("Cleaning up Cegui.");

  CEGUI::System::getSingleton().destroy();

  SetCeguiInitialized(api, false);
}

void SetAllCeguiVisibility(bool visible, bool /*old_visible*/)
{
  auto& console = GetGameConsoleWindow();
  console->SetVisible(visible);
}

void HandleInputQueueEntry(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (!CeguiInitializedAny())
  {
    return;
  }

  auto& window_interface = hadesmem::cerberus::GetWindowInterface();
  if (hwnd == window_interface.GetCurrentWindow())
  {
    switch (msg)
    {
    case WM_CHAR:
      CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(
        (CEGUI::utf32)wparam);
      break;

    case WM_MOUSEMOVE:
      CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(
        (float)(LOWORD(lparam)), (float)(HIWORD(lparam)));
      break;

    case WM_LBUTTONDOWN:
      CEGUI::System::getSingleton()
        .getDefaultGUIContext()
        .injectMouseButtonDown(CEGUI::LeftButton);
      break;

    case WM_LBUTTONUP:
      CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        CEGUI::LeftButton);
      break;

    case WM_RBUTTONDOWN:
      CEGUI::System::getSingleton()
        .getDefaultGUIContext()
        .injectMouseButtonDown(CEGUI::RightButton);
      break;

    case WM_RBUTTONUP:
      CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        CEGUI::RightButton);
      break;

    case WM_MBUTTONDOWN:
      CEGUI::System::getSingleton()
        .getDefaultGUIContext()
        .injectMouseButtonDown(CEGUI::MiddleButton);
      break;

    case WM_MBUTTONUP:
      CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        CEGUI::MiddleButton);
      break;

    case WM_MOUSEWHEEL:
      CEGUI::System::getSingleton()
        .getDefaultGUIContext()
        .injectMouseWheelChange(static_cast<float>((short)HIWORD(wparam)) /
                                static_cast<float>(120));
      break;
    }
  }
}

void OnFrameCegui(hadesmem::cerberus::RenderApi /*api*/, void* /*device*/)
{
  if (!CeguiInitializedAny())
  {
    return;
  }

  CEGUI::System::getSingleton().renderAllGUIContexts();
}

void OnResizeCegui(hadesmem::cerberus::RenderApi /*api*/,
                   void* /*device*/,
                   UINT width,
                   UINT height)
{
  if (!CeguiInitializedAny())
  {
    return;
  }

  // TODO: Reduce code duplication between this and other renderers.
  if (!width || !height)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("Size is zero, attempting to use client "
                                   "area of window. Width: [%u]. Height: [%u].",
                                   width,
                                   height);

    // TODO: Ensure we're using the right window.
    auto& window_interface = hadesmem::cerberus::GetWindowInterface();
    RECT rect{};
    if (::GetClientRect(window_interface.GetCurrentWindow(), &rect))
    {
      width = width ? width : rect.right;
      height = height ? height : rect.bottom;

      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Got client rect. Width: [%u]. Height: [%u].", width, height);
    }
    else
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_TRACE_FORMAT_A("GetClientRect failed. LastError: [%lu].",
                                     last_error);
    }
  }

  if (!width || !height)
  {
    HADESMEM_DETAIL_TRACE_A("Skipping resize due to unknown size.");
    return;
  }

  CEGUI::System::getSingleton().notifyDisplaySizeChanged(
    CEGUI::Sizef(width, height));
}

void OnUnloadPlugins()
{
  SetCeguiInitialized(hadesmem::cerberus::RenderApi::kD3D9, false);
  SetCeguiInitialized(hadesmem::cerberus::RenderApi::kD3D10, false);
  SetCeguiInitialized(hadesmem::cerberus::RenderApi::kD3D11, false);
  SetCeguiInitialized(hadesmem::cerberus::RenderApi::kOpenGL32, false);
}
}

namespace hadesmem
{
namespace cerberus
{
CeguiInterface& GetCeguiInterface() noexcept
{
  static CeguiImpl Cegui;
  return Cegui;
}

void InitializeCegui()
{
  auto& input = GetInputInterface();
  input.RegisterOnInputQueueEntry(HandleInputQueueEntry);

  auto& render = GetRenderInterface();
  render.RegisterOnFrame(OnFrameCegui);
  render.RegisterOnResize(OnResizeCegui);
  render.RegisterOnInitializeGui(OnInitializeCeguiGui);
  render.RegisterOnCleanupGui(OnCleanupCeguiGui);
  render.RegisterOnSetGuiVisibility(SetAllCeguiVisibility);

  RegisterOnUnloadPlugins(OnUnloadPlugins);
}
}
}
