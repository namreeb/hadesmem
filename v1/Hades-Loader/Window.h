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
#include "Game.h"
#include "Resource.h"

namespace Hades
{
  namespace Loader
  {
    // Loader window type
    template <typename T>
    class LoaderWindowT : 
      public CFrameWindowImpl<T, CWindow, 
      CWinTraits<WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
      WS_EX_APPWINDOW | WS_EX_WINDOWEDGE>>
    { };

    // Loader window manager
    class LoaderWindow : 
      public LoaderWindowT<LoaderWindow>, 
      public CUpdateUI<LoaderWindow>, 
      public CMessageFilter, 
      public CIdleHandler
    {
    public:
      // Specify window class name and resource id
      DECLARE_FRAME_WND_CLASS(L"HadesLoaderWndClass", IDR_LOADERWINDOW)

      // Constructor
      LoaderWindow(CAppModule* pAppModule);

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

      // ID_VIEW_STATUS_BAR command callback
      LRESULT OnViewStatusBar(WORD wNotifyCode, WORD wID, HWND hWndCtl, 
        BOOL& bHandled);

      // Create client area of window
      HWND CreateClient();

      // ATL message map
      BEGIN_MSG_MAP(LoaderWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        COMMAND_ID_HANDLER(ID_FILE_EXIT, OnFileExit)
        COMMAND_ID_HANDLER(ID_HELP_ABOUT, OnHelpAbout)
        COMMAND_ID_HANDLER(ID_GAMES_ADDGAME, OnMenuNotImpl)
        COMMAND_ID_HANDLER(ID_GAMES_REMOVEGAME, OnMenuNotImpl)
        COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
        CHAIN_MSG_MAP(CUpdateUI<LoaderWindow>)
        CHAIN_MSG_MAP(LoaderWindowT<LoaderWindow>)
      END_MSG_MAP()

      // ATL UI update map
      BEGIN_UPDATE_UI_MAP(LoaderWindow)
        UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
      END_UPDATE_UI_MAP()

      // Create test window
      CWindow CreateFooWindow() 
      {
        // Create test tree
        if (!m_FooTree.Create(m_TabView.m_hWnd, rcDefault, NULL, WS_CHILD | 
          WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE))
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("LoaderWindow::CreateTestWindow") << 
            ErrorString("Could not get create test tree.") << 
            ErrorCodeWin(LastError));
        }

        // Insert test item
        m_FooTree.InsertItem(L"Foo", TVI_ROOT, TVI_LAST);

        return m_FooTree;
      }

      // Create test window
      CWindow CreateBarWindow() 
      {
        // Create test tree
        if (!m_BarTree.Create(m_TabView.m_hWnd, rcDefault, NULL, WS_CHILD | 
          WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE))
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("LoaderWindow::CreateTestWindow") << 
            ErrorString("Could not get create test tree.") << 
            ErrorCodeWin(LastError));
        }

        // Insert test item
        m_BarTree.InsertItem(L"Bar", TVI_ROOT, TVI_LAST);

        return m_BarTree;
      }

    private:
      // App module
      CAppModule* m_pAppModule;

      // Game manager
      GameMgr m_GameMgr;

      // Tab view
      CTabView m_TabView;

      // Test trees
      CTreeViewCtrlEx m_FooTree;
      CTreeViewCtrlEx m_BarTree;
    };
  }
}
