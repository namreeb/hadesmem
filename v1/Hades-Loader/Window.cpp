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

// Hades
#include "About.h"
#include "Window.h"
#include "Resource.h"
#include "Hades-Common/Error.h"

namespace Hades
{
  namespace Loader
  {
    // WM_DESTROY message callback
    LRESULT LoaderWindow::OnDestroy(UINT /*nMsg*/, WPARAM /*wParam*/, 
      LPARAM /*lParam*/, BOOL& bHandled)
    {
      try
      {
        // Get message loop
        CMessageLoop* pLoop = m_pAppModule->GetMessageLoop();
        if (!pLoop)
        {
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("LoaderWindow::OnCreate") << 
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
          "Hades-Loader", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-Loader", MB_OK);
      }

      return 0;
    }

    // WM_COMMAND message callback
    LRESULT LoaderWindow::OnCommand(UINT /*nMsg*/, WPARAM wParam, 
      LPARAM /*lParam*/, BOOL& bHandled)
    {
      try
      {
        // Unhandled by default
        bHandled = FALSE;

        // Get message ID
        WORD MsgId = LOWORD(wParam);

        // Get game data for message id
        auto pGameData(m_GameMgr.GetDataForMessageId(MsgId));

        // If entry existed attempt to launch
        if (pGameData)
        {
          // Message handled
          bHandled = TRUE;

          // Launch game
          m_GameMgr.LaunchGame(*pGameData);
        }
      }
      catch (boost::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
          "Hades-Loader", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-Loader", MB_OK);
      }

      return 0;
    }

    // WM_CREATE message callback
    LRESULT LoaderWindow::OnCreate(UINT /*nMsg*/, WPARAM /*wParam*/, 
      LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
      try
      {
        // Resize window
        if (!ResizeClient(1280, 720))
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("LoaderWindow::OnCreate") << 
            ErrorString("Could not resize loader window.") << 
            ErrorCodeWin(LastError));
        }

        // Center window
        if (!CenterWindow())
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("LoaderWindow::OnCreate") << 
            ErrorString("Could not center loader window.") << 
            ErrorCodeWin(LastError));
        }

        // Create status bar
        if (!CreateSimpleStatusBar())
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("LoaderWindow::OnCreate") << 
            ErrorString("Could not create status bar.") << 
            ErrorCodeWin(LastError));
        }

        // Create window client area
        m_hWndClient = CreateClient();
        
        // Set status bar check
        UISetCheck(ID_VIEW_STATUS_BAR, 1);

        // Get message loop
        CMessageLoop* pLoop = m_pAppModule->GetMessageLoop();
        if (!pLoop)
        {
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("LoaderWindow::OnCreate") << 
            ErrorString("Could not get message loop."));
        }

        // Register for message filtering and idle updates
        pLoop->AddMessageFilter(this);
        pLoop->AddIdleHandler(this);

        // Get menu for window
        CMenuHandle MainMenu(GetMenu());
        if (MainMenu.IsNull())
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("LoaderWindow::OnCreate") << 
            ErrorString("Could not get main menu.") << 
            ErrorCodeWin(LastError));
        }

        // Get game menu
        CMenuHandle GameMenu(MainMenu.GetSubMenu(1));
        if (MainMenu.IsNull())
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("LoaderWindow::OnCreate") << 
            ErrorString("Could not get game menu.") << 
            ErrorCodeWin(LastError));
        }

        // Load game list
        m_GameMgr.LoadConfigDefault();

        // Get game list
        auto GameList(m_GameMgr.GetAllData());

        // Add all games to menu
        std::for_each(GameList.begin(), GameList.end(), 
          [&] (std::shared_ptr<GameMgr::MenuData> Current)
        {
          if (!GameMenu.AppendMenuW(MF_STRING, Current->MessageId, 
            Current->Name.c_str()))
          {
            DWORD LastError = GetLastError();
            BOOST_THROW_EXCEPTION(HadesError() << 
              ErrorFunction("LoaderWindow::OnCreate") << 
              ErrorString("Could not append game to game menu.") << 
              ErrorCodeWin(LastError));
          }
        });
      }
      catch (boost::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
          "Hades-Loader", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-Loader", MB_OK);
      }

      return 0;
    }

    // ID_FILE_EXIT command callback
    LRESULT LoaderWindow::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, 
      HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
      try
      {
        // Send close request
        if (!PostMessage(WM_CLOSE))
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("LoaderWindow::OnFileExit") << 
            ErrorString("Could not send close message.") << 
            ErrorCodeWin(LastError));
        }
      }
      catch (boost::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, boost::diagnostic_information(e).c_str(), 
          "Hades-Loader", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-Loader", MB_OK);
      }

      return 0;
    }

    // ID_HELP_ABOUT command callback
    LRESULT LoaderWindow::OnHelpAbout(WORD /*wNotifyCode*/, WORD /*wID*/, 
      HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
      // Create about dialog
      AboutDialog MyAboutDialog;
      // Show about dialog (modal)
      MyAboutDialog.DoModal();

      return 0;
    }

    // Create client area of window
    HWND LoaderWindow::CreateClient()
    {
      // Create tab view
      if (!m_TabView.Create(*this, rcDefault, NULL, WS_CHILD | 
        WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("LoaderWindow::CreateClient") << 
          ErrorString("Could not create tab view.") << 
          ErrorCodeWin(LastError));
      }

      // Set up tab view
      m_TabView.AddPage(CreateFooWindow(), L"Foo");
      m_TabView.AddPage(CreateBarWindow(), L"Bar");
      m_TabView.SetActivePage(0);

      // Return tab view handle as client
      return m_TabView.m_hWnd;
    }

    // PreTranslateMessage handler (CMessageFilter)
    BOOL LoaderWindow::PreTranslateMessage(MSG* pMsg)
    {
      return LoaderWindowT<LoaderWindow>::PreTranslateMessage(pMsg);
    }

    // Idle handler (CIdleHandler)
    BOOL LoaderWindow::OnIdle()
    {
      return FALSE;
    }

    // Constructor
    LoaderWindow::LoaderWindow(CAppModule* pAppModule) 
      : m_pAppModule(pAppModule), 
      m_GameMgr(), 
      m_FooTree(), 
      m_BarTree()
    { }

    // ID_VIEW_STATUS_BAR command callback
    LRESULT LoaderWindow::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, 
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
          "Hades-Loader", MB_OK);
      }
      catch (std::exception const& e)
      {
        // Dump error information
        MessageBoxA(NULL, e.what(), "Hades-Loader", MB_OK);
      }

      return 0;
    }

    // Menu command callback for features that are unavailable
    LRESULT LoaderWindow::OnMenuNotImpl(WORD /*wNotifyCode*/, WORD /*wID*/, 
      HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
      MessageBox(L"This feature is currently unavailable.", L"Hades-Loader");
      return 0;
    }
  }
}
