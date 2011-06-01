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

#pragma once

// C++ Standard Library
#include <memory>

// Windows
#include <Windows.h>
#include <atlbase.h>
#include <atlwin.h>

// DirectX
#include <d3d9.h>
#include <d3dx9.h>

// WTL
#include <atlapp.h>
#include <atlddx.h>
#include <atluser.h>
#include <atlmisc.h>
#include <atlframe.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlsplit.h>
#include <atlctrlx.h>

// Hades
#include "Resource.h"
#include "GameLoop.h"
#include "Hades-GUI/GUI.h"

namespace Hades
{
  namespace GUISandbox
  {
    // Forward declaration
    class SandboxWindow;

    // GUI sandbox game handler for GameLoop
    class SandboxGameHandler : public GameHandler
    {
    public:
      // Constructor
      SandboxGameHandler(SandboxWindow* pSandboxWindow);

      // Whether sandbox is currently paused
      virtual BOOL IsPaused();

      // OnFrame callback for GameLoop
      virtual HRESULT OnUpdateFrame();

    private:
      // Sandbox window
      SandboxWindow* m_pSandboxWindow;
    };

    // GUI sandbox window manager
    class SandboxWindow : 
      public CFrameWindowImpl<SandboxWindow>, 
      public CUpdateUI<SandboxWindow>, 
      public CMessageFilter, 
      public CIdleHandler
    {
    public:
      // Specify window class name and resource id
      DECLARE_FRAME_WND_CLASS(L"HadesGUISandboxWndClass", IDR_HADES_GUISANDBOX)

      // Constructor
      SandboxWindow(CAppModule* pAppModule);

      // Initialize D3D
      void InitD3D();

      // Load GUI
      void LoadGUI();

      // Render D3D screen
      void RenderScreen();

    private:
      // PreTranslateMessage handler (CMessageFilter)
      virtual BOOL PreTranslateMessage(MSG* pMsg);

      // Idle handler (CIdleHandler)
      virtual BOOL OnIdle();

      // WM_DESTROY message callback
      LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, 
        BOOL& bHandled);

      // WM_CREATE message callback
      LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, 
        BOOL& bHandled);

      // WM_COMMAND message callback
      LRESULT OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, 
        BOOL& bHandled);

      // ID_FILE_EXIT command callback
      LRESULT OnFileExit(WORD wNotifyCode, WORD wID, HWND hWndCtl, 
        BOOL& bHandled);

      // ID_HELP_ABOUT command callback
      LRESULT OnHelpAbout(WORD wNotifyCode, WORD wID, HWND hWndCtl, 
        BOOL& bHandled);

      // Menu command callback for features that are unavailable
      LRESULT OnMenuNotImpl(WORD wNotifyCode, WORD wID, HWND hWndCtl, 
        BOOL& bHandled);

      // TVN_SELCHANGED notification callback
      LRESULT OnTVSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

      // ID_VIEW_STATUS_BAR command callback
      LRESULT OnViewStatusBar(WORD wNotifyCode, WORD wID, HWND hWndCtl, 
        BOOL& bHandled);

      // ID_RENDERING_RESET command callback
      LRESULT OnResetGUI(WORD wNotifyCode, WORD wID, HWND hWndCtl, 
        BOOL& bHandled);

      // ID_RENDERING_RELEASE command callback
      LRESULT OnReloadGUI(WORD wNotifyCode, WORD wID, HWND hWndCtl, 
        BOOL& bHandled);

      // Create client area of window
      HWND CreateClient();

      // Create GUI menu
      GUI::Window* CreateMainMenu();

      // Add item to GUI menu
      void AddMainMenuItem(GUI::Window* pWindow, std::string const& Label, 
        std::string const& Window);

      // GUI menu callback
      std::string MainMenuCallback(const char * pArgs, GUI::Element* pElement);

      // ATL message map
      BEGIN_MSG_MAP(SandboxWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_ID_HANDLER(ID_FILE_EXIT, OnFileExit)
        COMMAND_ID_HANDLER(ID_HELP_ABOUT, OnHelpAbout)
        COMMAND_ID_HANDLER(ID_RENDERING_RESET, OnResetGUI)
        COMMAND_ID_HANDLER(ID_RENDERING_RELEASE, OnReloadGUI)
        COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
        CHAIN_MSG_MAP(CUpdateUI<SandboxWindow>)
        CHAIN_MSG_MAP(CFrameWindowImpl<SandboxWindow>)
      END_MSG_MAP()

      // ATL UI update map
      BEGIN_UPDATE_UI_MAP(SandboxWindow)
        UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
      END_UPDATE_UI_MAP()

      // Game handler
      SandboxGameHandler m_GameHandler;

      // App module
      CAppModule* m_pAppModule;

      // D3D presentation params
      D3DPRESENT_PARAMETERS m_D3DPresentParams;

      // D3D interface
      CComPtr<IDirect3D9> m_pD3D;
      // D3D device
      CComPtr<IDirect3DDevice9> m_pDevice;

      // D3D reset flag
      bool m_Reset;

      // FPS
      DWORD m_FPS;
      // FPS timer
      DWORD m_FPSTimer;

      // GUI instance
      std::shared_ptr<GUI::GUI> m_pGUI;
      
      // Menu size
      int m_MenuSize;
    };
  }
}
