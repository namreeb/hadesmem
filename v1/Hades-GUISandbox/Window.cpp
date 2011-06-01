/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

// Boost
#include <boost/format.hpp>

// Hades
#include "About.h"
#include "Window.h"
#include "Resource.h"
#include "Hades-Common/Error.h"
#include "Hades-Common/Filesystem.h"
#include "Hades-Common/StringBuffer.h"

namespace Hades
{
  namespace GUISandbox
  {
    // WM_DESTROY message callback
    LRESULT SandboxWindow::OnDestroy(UINT /*nMsg*/, WPARAM /*wParam*/, 
      LPARAM /*lParam*/, BOOL& bHandled)
    {
      try
      {
        // Get message loop
        CMessageLoop* pLoop = m_pAppModule->GetMessageLoop();
        if (!pLoop)
        {
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("SandboxWindow::OnCreate") << 
            ErrorString("Could not get message loop."));
        }

        // Register for message filtering and idle updates
        pLoop->RemoveMessageFilter(this);
        pLoop->RemoveIdleHandler(this);

        // Mark as unhandled so next in chain is called
        bHandled = FALSE;
      }
      catch (boost::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
          "Hades-GUISandbox", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-GUISandbox", MB_OK);
      }

      return 0;
    }

    // WM_CREATE message callback
    LRESULT SandboxWindow::OnCreate(UINT /*nMsg*/, WPARAM /*wParam*/, 
      LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
      try
      {
        // Resize window
        if (!ResizeClient(1280, 720))
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("SandboxWindow::OnCreate") << 
            ErrorString("Could not resize loader window.") << 
            ErrorCodeWin(LastError));
        }

        // Center window
        if (!CenterWindow())
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("SandboxWindow::OnCreate") << 
            ErrorString("Could not center loader window.") << 
            ErrorCodeWin(LastError));
        }

        // Create status bar
        if (!CreateSimpleStatusBar())
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("SandboxWindow::OnCreate") << 
            ErrorString("Could not create status bar.") << 
            ErrorCodeWin(LastError));
        }

        // Set status bar check
        UISetCheck(ID_VIEW_STATUS_BAR, 1);

        // Get message loop
        auto pLoop = dynamic_cast<GameLoop*>(m_pAppModule->GetMessageLoop());
        if (!pLoop)
        {
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("SandboxWindow::OnCreate") << 
            ErrorString("Could not get message loop."));
        }

        // Set game handler
        pLoop->SetGameHandler(&m_GameHandler);

        // Register for message filtering and idle updates
        pLoop->AddMessageFilter(this);
        pLoop->AddIdleHandler(this);
      }
      catch (boost::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
          "Hades-GUISandbox", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-GUISandbox", MB_OK);
      }

      return 0;
    }

    // ID_FILE_EXIT command callback
    LRESULT SandboxWindow::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, 
      HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
      try
      {
        // Send close request
        if (!PostMessage(WM_CLOSE))
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("SandboxWindow::OnFileExit") << 
            ErrorString("Could not send close message.") << 
            ErrorCodeWin(LastError));
        }
      }
      catch (boost::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
          "Hades-GUISandbox", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-GUISandbox", MB_OK);
      }

      return 0;
    }

    // ID_HELP_ABOUT command callback
    LRESULT SandboxWindow::OnHelpAbout(WORD /*wNotifyCode*/, WORD /*wID*/, 
      HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
      // Create about dialog
      AboutDialog MyAboutDialog;
      // Show about dialog (modal)
      MyAboutDialog.DoModal();

      return 0;
    }

    // PreTranslateMessage handler (CMessageFilter)
    BOOL SandboxWindow::PreTranslateMessage(MSG* pMsg)
    {
      // Get message params
      UINT uMsg = pMsg->message;
      WPARAM wParam = pMsg->wParam;
      LPARAM lParam = pMsg->lParam;

      // If GUI handled message then there's nothing left to do
      if(m_pGUI && (m_pGUI->GetMouse().HandleMessage(uMsg, wParam, lParam) || 
        m_pGUI->GetKeyboard().HandleMessage(uMsg, wParam, lParam)))
      {
        return TRUE;
      }

      // Call next in chain
      return CFrameWindowImpl<SandboxWindow>::PreTranslateMessage(pMsg);
    }

    // Idle handler (CIdleHandler)
    BOOL SandboxWindow::OnIdle()
    {
      return FALSE;
    }

    // Constructor
    SandboxWindow::SandboxWindow(CAppModule* pAppModule) 
      : m_GameHandler(this), 
      m_pAppModule(pAppModule), 
      m_D3DPresentParams(), 
      m_pD3D(), 
      m_pDevice(), 
      m_Reset(false), 
      m_FPS(0), 
      m_FPSTimer(0), 
      m_pGUI(), 
      m_MenuSize(0)
    {
      // Reset presentation params
      ZeroMemory(&m_D3DPresentParams, sizeof(m_D3DPresentParams));
    }

    // ID_VIEW_STATUS_BAR command callback
    LRESULT SandboxWindow::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, 
      HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
      try
      {
        CWindow StatusBarWnd(m_hWndStatusBar);
        BOOL bVisible = !StatusBarWnd.IsWindowVisible();
        StatusBarWnd.ShowWindow(bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
        UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
        UpdateLayout();
      }
      catch (boost::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
          "Hades-GUISandbox", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-GUISandbox", MB_OK);
      }

      return 0;
    }

    // Menu command callback for features that are unavailable
    LRESULT SandboxWindow::OnMenuNotImpl(WORD /*wNotifyCode*/, WORD /*wID*/, 
      HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
      MessageBox(L"This feature is currently unavailable.", 
        L"Hades-GUISandbox");
      return 0;
    }

    // Initialize D3D
    void SandboxWindow::InitD3D()
    {
      // Create D3D
      m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
      if (!m_pD3D)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("SandboxWindow::InitD3D") << 
          ErrorString("Could not create D3D.") << 
          ErrorCodeWin(LastError));
      }

      // Reset presentation params
      ZeroMemory(&m_D3DPresentParams, sizeof(m_D3DPresentParams));

      // Initialize presentation params
      m_D3DPresentParams.BackBufferCount = 1;
      m_D3DPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
      m_D3DPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
      m_D3DPresentParams.hDeviceWindow = *this;
      m_D3DPresentParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
      m_D3DPresentParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
      m_D3DPresentParams.BackBufferFormat = D3DFMT_R5G6B5;
      m_D3DPresentParams.Windowed = TRUE;

      // Create device
      HRESULT Result = m_pD3D->CreateDevice(
        D3DADAPTER_DEFAULT, 
        D3DDEVTYPE_HAL, 
        *this, 
        D3DCREATE_HARDWARE_VERTEXPROCESSING, 
        &m_D3DPresentParams, 
        &m_pDevice.p);
      if (FAILED(Result))
      {
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("SandboxWindow::InitD3D") << 
          ErrorString("Could not create D3D device.") << 
          ErrorCodeWin(Result));
      }
    }

    // Render D3D screen
    void SandboxWindow::RenderScreen()
    {
      // Sanity checks
      if (!m_pD3D || !m_pDevice)
      {
        return;
      }

      // If a second has passed update FPS title bar counter
      if(GetTickCount() - m_FPSTimer >= 1000)
      {
        // Update title bar
        std::wstring const NewTitle((boost::wformat(L"Hades GUI Sandbox. "
          L"FPS: %u.") %m_FPS).str());
        SetWindowText(NewTitle.c_str());

        // Reset FPS count and timer
        m_FPS = 0;
        m_FPSTimer = GetTickCount();
      }

      // Ensure device is in a valid state
      if(!FAILED(m_pDevice->TestCooperativeLevel()) && !m_Reset)
      {
        // Clear render target
        m_pDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 
          1.0f, 0);

        // Begin render scene
        m_pDevice->BeginScene();

        // Render GUI if instance exists
        if(m_pGUI)
        {
          m_pGUI->Draw();
        }

        // End render scene
        m_pDevice->EndScene();

        // Present buffer
        m_pDevice->Present(0, 0, 0, 0);

        // Increase frame count
        ++m_FPS;
      }
    }

    // Load GUI
    void SandboxWindow::LoadGUI()
    {
      // Create GUI
      m_pGUI.reset(new GUI::GUI(m_pDevice));

      // Get current working directory
      std::wstring CurDir;
      if (!GetCurrentDirectory(MAX_PATH, Util::MakeStringBuffer(CurDir, 
        MAX_PATH)))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("SandboxWindow::LoadGUI") << 
          ErrorString("Could not get current directory.") << 
          ErrorCodeWin(LastError));
      }

      // Set new working directory to GUI library folder
      std::wstring GuiPath((Windows::GetSelfDirPath() / L"/GUI/").
        file_string());
      if (!SetCurrentDirectory(GuiPath.c_str()))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("SandboxWindow::LoadGUI") << 
          ErrorString("Could not set current directory.") << 
          ErrorCodeWin(LastError));
      }

      // Load interface data
      m_pGUI->LoadInterfaceFromFile("ColorThemes.xml");
      m_pGUI->LoadInterfaceFromFile("GuiTest_UI.xml");

      // Restore old working directory
      if (!SetCurrentDirectory(CurDir.c_str()))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("SandboxWindow::LoadGUI") << 
          ErrorString("Could not restore current directory.") << 
          ErrorCodeWin(LastError));
      }

      // Create main menu
      auto pMainMenu = CreateMainMenu();
      AddMainMenuItem(pMainMenu, "Waypoints", "WINDOW_WAYPOINT_CONTROLS");
      AddMainMenuItem(pMainMenu, "Test 2", "WINDOW_TEST2");

      // Set main menu as visible
      pMainMenu->SetVisible(true);

      // Set GUI as visible
      m_pGUI->SetVisible(true);
    }

    // ID_RENDERING_RESET command callback
    LRESULT SandboxWindow::OnResetGUI(WORD /*wNotifyCode*/, WORD /*wID*/, 
      HWND /*hWndCtl*/, BOOL& bHandled)
    {
      try
      {
        // Event handled
        bHandled = TRUE;

        // Ensure GUI instance exists
        if (m_pGUI)
        {
          // Set reset flag
          m_Reset = true;
          do
          {
            // Notify GUI of lost device
            m_pGUI->OnLostDevice();
            // Reset device
            m_pDevice->Reset(&m_D3DPresentParams);
            // Notify GUI of reset device
            m_pGUI->OnResetDevice(m_pDevice);
          }
          // Ensure device has been reset
          while(FAILED(m_pDevice->TestCooperativeLevel()));
          // Unset reset flag
          m_Reset = false;
        }
      }
      catch (boost::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
          "Hades-GUISandbox", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-GUISandbox", MB_OK);
      }

      return 0;
    }

    // ID_RENDERING_RELEASE command callback
    LRESULT SandboxWindow::OnReloadGUI(WORD /*wNotifyCode*/, WORD /*wID*/, 
      HWND /*hWndCtl*/, BOOL& bHandled)
    {
      try
      {
        // Event handled
        bHandled = TRUE;

        // Ensure GUI instance exists
        if(m_pGUI)
        {
          // Delete GUI instance
          m_pGUI.reset();

          // Create new GUI instance
          LoadGUI();
        }
      }
      catch (boost::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
          "Hades-GUISandbox", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-GUISandbox", MB_OK);
      }

      return 0;
    }

    // Create GUI menu
    GUI::Window* SandboxWindow::CreateMainMenu()
    {
      // Create XML element to initialize GUI element
      auto pElement(std::make_shared<TiXmlElement>("Window"));
      pElement->SetAttribute("absX", 100);
      pElement->SetAttribute("absY", 330);
      pElement->SetAttribute("width", 130);
      pElement->SetAttribute("height", 40);
      pElement->SetAttribute("string", "Main menu");
      pElement->SetAttribute("string2", "WINDOW_MAIN");
      pElement->SetAttribute("visible", 0);
      pElement->SetAttribute("closebutton", 1);

      // Reset menu size
      m_MenuSize = 0;

      // Create main menu window
      auto pWindow = m_pGUI->AddWindow(new GUI::Window(*m_pGUI, &*pElement));

      // Return pointer to window
      return pWindow;
    }

    // Add item to GUI menu
    void SandboxWindow::AddMainMenuItem(GUI::Window* pWindow, 
      std::string const& Label, std::string const& Window)
    {
      // Increase menu size
      pWindow->SetHeight(pWindow->GetHeight() + 25);

      // Create new menu button
      auto pButton = new Hades::GUI::Button(*m_pGUI, 0);
      pButton->SetHeight(BUTTON_HEIGHT);
      pButton->SetWidth(100);
      pButton->SetRelPos(Hades::GUI::Pos(15, m_MenuSize * 25 + 10));
      pButton->SetString(Label);
      pButton->SetString(Window, 1);
      pButton->SetCallback(std::bind(&SandboxWindow::MainMenuCallback, this, 
        std::placeholders::_1, std::placeholders::_2));

      // Add button to menu
      pWindow->AddElement(pButton);

      // Increase menu count
      ++m_MenuSize;
    }

    // GUI menu callback
    std::string SandboxWindow::MainMenuCallback(const char* /*pArgs*/, 
      GUI::Element* pElement )
    {
      // Get window corresponding to selected menu item
      if(auto pWindow = m_pGUI->GetWindowByString(pElement->GetString(false, 
        1), 1))
      {
        // Show window
        pWindow->SetVisible(!pWindow->IsVisible());
      }

      return std::string();
    }

    // Constructor
    SandboxGameHandler::SandboxGameHandler(SandboxWindow* pSandboxWindow) 
      : m_pSandboxWindow(pSandboxWindow)
    { }

    // Whether sandbox is currently paused
    // TODO: Implement this
    BOOL SandboxGameHandler::IsPaused()
    {
      return FALSE;
    }

    // OnFrame callback for GameLoop. Calls RenderScreen for sandbox 
    // window.
    HRESULT SandboxGameHandler::OnUpdateFrame()
    {
      try
      {
        // Render screen
        m_pSandboxWindow->RenderScreen();

        // Success
        return S_OK;
      }
      catch (boost::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
          "Hades-GUISandbox", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-GUISandbox", MB_OK);
      }

      // Something went wrong
      return S_FALSE;
    }
  }
}
