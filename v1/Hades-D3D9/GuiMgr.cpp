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

// C++ Standard Library
#include <iostream>

// Windows
#include <atlbase.h>
#include <atlwin.h>

// Hades
#include "GuiMgr.h"
#include "D3D9Mgr.h"
#include "Hades-Input/InputMgr.h"
#include "Hades-Common/Filesystem.h"
#include "Hades-Common/StringBuffer.h"

namespace Hades
{
  namespace D3D9
  {
    // Constructor
    GuiMgr::GuiMgr(Kernel::Kernel* pKernel) 
      : m_pGui(nullptr), 
      m_pKernel(pKernel), 
      m_pDevice(nullptr), 
      m_CursorX(0), 
      m_CursorY(0), 
      m_CallsOnConsoleInput(), 
      m_GuiMutex(), 
      m_ConsoleHistory(), 
      m_HistoryPos(0), 
      m_Watermark(true)
    {
      // Register for D3D events
      D3D9Mgr::RegisterOnInitialize(std::bind(&GuiMgr::OnInitialize, this, 
        std::placeholders::_1, std::placeholders::_2));
      D3D9Mgr::RegisterOnFrame(std::bind(&GuiMgr::OnFrame, this, 
        std::placeholders::_1, std::placeholders::_2));
      D3D9Mgr::RegisterOnLostDevice(std::bind(&GuiMgr::OnLostDevice, this, 
        std::placeholders::_1, std::placeholders::_2));
      D3D9Mgr::RegisterOnResetDevice(std::bind(&GuiMgr::OnResetDevice, this, 
        std::placeholders::_1, std::placeholders::_2));
      D3D9Mgr::RegisterOnRelease(std::bind(&GuiMgr::OnRelease, this, 
        std::placeholders::_1, std::placeholders::_2));

      // Register for input events
      pKernel->GetInputMgr()->RegisterOnWindowMessage(std::bind(
        &GuiMgr::OnInputMsg, this, std::placeholders::_1, std::placeholders::_2, 
        std::placeholders::_3, std::placeholders::_4));
      pKernel->GetInputMgr()->RegisterOnSetCursor(std::bind(&GuiMgr::OnSetCursor, 
        this, std::placeholders::_1));
      pKernel->GetInputMgr()->RegisterOnGetCursorPos(std::bind(
        &GuiMgr::OnGetCursorPos, this, std::placeholders::_1));
      pKernel->GetInputMgr()->RegisterOnSetCursorPos(std::bind(
        &GuiMgr::OnSetCursorPos, this, std::placeholders::_1, 
        std::placeholders::_2));

      // Notify kernel of creation
      m_pKernel->SetGuiMgr(this);
    }

    // Initialize GUI from device
    void GuiMgr::OnInitialize(IDirect3DDevice9* pDevice, 
      D3D9HelperPtr /*pHelper*/)
    {
      // Delete GUI instance if it already exists
      if (m_pGui)
      {
        delete m_pGui;
        m_pGui = nullptr;
      }

      // Set device pointer
      m_pDevice = pDevice;

      // Create new GUI instance
      m_pGui = new GUI::GUI(pDevice);

      // Get current working directory
      std::wstring CurDir;
      if (!GetCurrentDirectory(MAX_PATH, Util::MakeStringBuffer(CurDir, 
        MAX_PATH)))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(GuiMgrError() << 
          ErrorFunction("GuiMgr::OnInitialize") << 
          ErrorString("Could not get current directory.") << 
          ErrorCodeWin(LastError));
      }

      // Set new working directory to GUI library folder
      std::wstring GuiPath((Windows::GetSelfDirPath() / L"/Gui/").
        file_string());
      if (!SetCurrentDirectory(GuiPath.c_str()))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(GuiMgrError() << 
          ErrorFunction("GuiMgr::OnInitialize") << 
          ErrorString("Could not set current directory.") << 
          ErrorCodeWin(LastError));
      }

      // Load GUI
      m_pGui->LoadInterfaceFromFile("ColorThemes.xml");
      m_pGui->LoadInterfaceFromFile("Console.xml");

      // Set callbacks
      auto pConsole = m_pGui->GetWindowByString("HADES_CONSOLE_WINDOW", 1);
      if (pConsole)
      {
        auto pInBox = pConsole->GetElementByString("HADES_CONSOLE_INPUT", 1);
        if (pInBox)
        {
          pInBox->SetCallback(std::bind(&GuiMgr::OnConsoleInput, this, 
            std::placeholders::_1, std::placeholders::_2));
        }
        else
        {
          std::wcout << "GuiMgr::OnInitialize: Warning! Could not find "
            "console input box." << std::endl;
        }
      }
      else
      {
        std::wcout << "GuiMgr::OnInitialize: Warning! Could not find console "
          "window." << std::endl;
      }

      // Show GUI
      m_pGui->SetVisible(true);

      // Restore old working directory
      if (!SetCurrentDirectory(CurDir.c_str()))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(GuiMgrError() << 
          ErrorFunction("GuiMgr::OnInitialize") << 
          ErrorString("Could not restore current directory.") << 
          ErrorCodeWin(LastError));
      }
    }

    // D3D9Mgr OnFrame callback
    void GuiMgr::OnFrame(IDirect3DDevice9* pDevice, D3D9HelperPtr pHelper)
    {
//       // Update window title
//       CWindow MyWindow(m_pKernel->GetD3D9Mgr()->GetDeviceWindow());
//       if (!MyWindow.SetWindowTextW(m_pKernel->GetSessionName().c_str()))
//       {
//         DWORD LastError = GetLastError();
//         BOOST_THROW_EXCEPTION(GuiMgrError() << 
//           ErrorFunction("GuiMgr::OnFrame") << 
//           ErrorString("Could not set window text.") << 
//           ErrorCodeWin(LastError));
//       }

      // Ensure GUI is valid
      if (!m_pGui)
      {
        return;
      }

      {
        // Lock GUI mutex
        boost::lock_guard<boost::mutex> GuiLock(m_GuiMutex);

        // Draw GUI
        m_pGui->Draw();
      }

      // Watermark if enabled
      if (m_Watermark)
      {
        // Get viewport
        D3DVIEWPORT9 Viewport;
        pDevice->GetViewport(&Viewport);

        // Draw box on viewport border
        Hades::Math::Vec2f const TopLeft(0,0);
        Hades::Math::Vec2f const BottomRight(static_cast<float>(Viewport.Width), 
          static_cast<float>(Viewport.Height));
        pHelper->DrawBox(TopLeft, BottomRight, 2, D3DCOLOR_ARGB(255, 0, 255, 0));

        // Draw test string
        GUI::Colour MyColor(255, 0, 0, 255);
        m_pGui->GetFont().DrawString(Viewport.X + 10, Viewport.Y + 10, 0, 
          &MyColor, L"Hades");
      }
    }

    // D3D9Mgr OnLostDevice callback
    void GuiMgr::OnLostDevice(IDirect3DDevice9* /*pDevice*/, 
      D3D9HelperPtr /*pHelper*/)
    {
      if (!m_pGui)
      {
        return;
      }

      m_pGui->OnLostDevice();
    }

    // D3D9Mgr OnResetDevice callback
    void GuiMgr::OnResetDevice(IDirect3DDevice9* pDevice, 
      D3D9HelperPtr /*pHelper*/)
    {
      if (!m_pGui)
      {
        return;
      }

      m_pGui->OnResetDevice(pDevice);
    }

    // D3D9Mgr OnRelease callback
    void GuiMgr::OnRelease(IDirect3DDevice9* /*pDevice*/, 
      D3D9HelperPtr /*pHelper*/)
    {
    }

    // InputMgr OnInputMsg callback
    bool GuiMgr::OnInputMsg(HWND /*hwnd*/, UINT uMsg, WPARAM wParam, 
      LPARAM lParam)
    {
      // Nothing to do if there is no current GUI instance
      if (!m_pGui)
      {
        return true;
      }

      // Toggle GUI on F12
      bool GuiToggled = false;
      if (uMsg == WM_KEYDOWN && wParam == VK_F12)
      {
        // Toggle visibility
        ToggleVisible();
        GuiToggled = true;

        // Get console window
        auto pConsole = m_pGui->GetWindowByString("HADES_CONSOLE_WINDOW", 1);
        if (!pConsole)
        {
          std::wcout << "GuiMgr::Print: Warning! Could not find console "
            "window." << std::endl;
        }
        else
        {
          // Get input box
          auto pInBox = pConsole->GetElementByString("HADES_CONSOLE_INPUT", 1);
          if (!pInBox)
          {
            std::wcout << "GuiMgr::Print: Warning! Could not find console "
              "input box." << std::endl;
          }
          else
          {
            // Set input box as focused element
            auto pInBoxReal = dynamic_cast<GUI::EditBox*>(pInBox);
            pInBoxReal->GetParent()->SetFocussedElement(pInBoxReal);
          }
        }
      }

      // Check for Up/Down keys for console history support
      if (uMsg == WM_KEYDOWN && (wParam == VK_UP || wParam == VK_DOWN))
      {
        // Get console window
        auto pConsole = m_pGui->GetWindowByString("HADES_CONSOLE_WINDOW", 1);
        if (!pConsole)
        {
          std::wcout << "GuiMgr::Print: Warning! Could not find console "
            "window." << std::endl;
        }
        else
        {
          // Get input box
          auto pInBox = pConsole->GetElementByString("HADES_CONSOLE_INPUT", 1);
          if (!pInBox)
          {
            std::wcout << "GuiMgr::Print: Warning! Could not find console "
              "input box." << std::endl;
          }
          else
          {
            // Check if input box is focused element
            auto pInBoxReal = dynamic_cast<GUI::EditBox*>(pInBox);
            if (pInBoxReal == pConsole->GetFocussedElement())
            {
              // Handle up key
              if (wParam == VK_UP)
              {
                // Get new history position
                m_HistoryPos = std::max<long>(m_HistoryPos - 1, 
                  static_cast<long>(-1));

                // If history position is valid display target history entry, 
                // otherwise clear the input box
                if (m_HistoryPos >= 0)
                {
                  pInBoxReal->SetString(m_ConsoleHistory[m_HistoryPos]);
                  pInBoxReal->SetStart(0);
                  pInBoxReal->SetIndex(0);
                }
                else
                {
                  pInBoxReal->SetString("");
                  pInBoxReal->SetStart(0);
                  pInBoxReal->SetIndex(0);
                }
              }
              // Handle down key
              else if (wParam == VK_DOWN)
              {
                // Get new history position
                m_HistoryPos = std::min<long>(m_HistoryPos + 1, 
                  static_cast<long>(m_ConsoleHistory.size()));

                // If history position is valid display target history entry, 
                // otherwise clear the input box
                if (m_HistoryPos < static_cast<long>(m_ConsoleHistory.size()))
                {
                  pInBoxReal->SetString(m_ConsoleHistory[m_HistoryPos]);
                  pInBoxReal->SetStart(0);
                  pInBoxReal->SetIndex(0);
                }
                else
                {
                  pInBoxReal->SetString("");
                  pInBoxReal->SetStart(0);
                  pInBoxReal->SetIndex(0);
                }
              }
            }
          }
        }
      }

      // Notify GUI of input events
      m_pGui->GetMouse().HandleMessage(uMsg, wParam, lParam);
      m_pGui->GetKeyboard().HandleMessage(uMsg, wParam, lParam);

      // Block input when GUI is visible
      return !((m_pGui->IsVisible() || GuiToggled) && 
        (uMsg == WM_CHAR || 
        uMsg == WM_KEYDOWN || 
        uMsg == WM_KEYUP || 
        uMsg == WM_MOUSEMOVE || 
        uMsg == WM_LBUTTONDOWN || 
        uMsg == WM_RBUTTONDOWN || 
        uMsg == WM_MBUTTONDOWN || 
        uMsg == WM_LBUTTONUP || 
        uMsg == WM_RBUTTONUP || 
        uMsg == WM_MBUTTONUP || 
        uMsg == WM_RBUTTONDBLCLK || 
        uMsg == WM_LBUTTONDBLCLK || 
        uMsg == WM_MBUTTONDBLCLK || 
        uMsg == WM_MOUSEWHEEL));
    }

    // InputMgr OnSetCursor callback
    bool GuiMgr::OnSetCursor(HCURSOR hCursor)
    {
      // Only allow cursor to be modified when it's not being removed entirely 
      // and the GUI is not visible
      return !(hCursor == NULL && m_pGui && m_pGui->IsVisible());
    }

    // Toggle GUI's visibility
    void GuiMgr::ToggleVisible()
    {
      // Previous cursor state
      static HCURSOR PrevCursor = NULL;

      // If GUI is visible
      if (m_pGui->IsVisible()) 
      {
        // Hide GUI 
        m_pGui->SetVisible(false);

        // Restore previous cursor
        PrevCursor = SetCursor(PrevCursor);
      }
      else
      {
        // Hide GUI 
        m_pGui->SetVisible(true);

        // Load default cursor
        HCURSOR DefArrow = LoadCursor(NULL, IDC_ARROW);
        // Set cursor
        PrevCursor = SetCursor(DefArrow);
      }
    }

    // InputMgr OnGetCursorPos callback
    bool GuiMgr::OnGetCursorPos(LPPOINT lpPoint)
    {
      // Use cached values and block call if GUI is currently visible
      if (m_pGui && m_pGui->IsVisible() && (m_CursorX != 0 || m_CursorY != 0))
      {
        lpPoint->x = m_CursorX;
        lpPoint->y = m_CursorY;

        return false;
      }

      return true;
    }

    // InputMgr OnSetCursorPos callback
    bool GuiMgr::OnSetCursorPos(int X, int Y)
    {
      // Backup current values and block call if GUI is currently visible
      if (m_pGui && m_pGui->IsVisible())
      {
        m_CursorX = X;
        m_CursorY = Y;

        return false;
      }

      return true;
    }

    // GUI library callback for console input
    std::string GuiMgr::OnConsoleInput(char const* pszArgs, 
      GUI::Element* pElement)
    {
      // Get input
      std::string const Input(pszArgs);

      // Notify subscribers
      m_CallsOnConsoleInput(Input);

      // Clear input box and refocus
      auto pEditBox = dynamic_cast<GUI::EditBox*>(pElement);
      pEditBox->SetString("");
      pEditBox->SetStart(0);
      pEditBox->SetIndex(0);
      pEditBox->GetParent()->SetFocussedElement(pElement);

      // Add current entry to history
      m_ConsoleHistory.push_back(pszArgs);
      m_HistoryPos = static_cast<long>(m_ConsoleHistory.size());

      // Debug output
      std::cout << "GuiMgr::OnConsoleInput: Input = \"" << pszArgs << "\"." << 
        std::endl;

      // Forced return value
      return std::string();
    }

    // Print output to console
    void GuiMgr::Print(std::string const& Output)
    {
      // Get console window
      auto pConsole = m_pGui->GetWindowByString("HADES_CONSOLE_WINDOW", 1);
      if (!pConsole)
      {
        std::wcout << "GuiMgr::Print: Warning! Could not find console "
          "window." << std::endl;
        return;
      }

      // Get input box
      auto pOutBox = pConsole->GetElementByString("HADES_CONSOLE_OUTPUT", 1);
      if (!pOutBox)
      {
        std::wcout << "GuiMgr::Print: Warning! Could not find console output "
          "box." << std::endl;
        return;
      }

      // Add output
      auto pOutBoxReal = dynamic_cast<GUI::TextBox*>(pOutBox);
      pOutBoxReal->AddString(Output);

      // Debug output
      std::cout << "GuiMgr::Print: Output = \"" << Output << "\"." << 
        std::endl;
    }

    // Register callback for OnConsoleInput event
    boost::signals2::connection GuiMgr::RegisterOnConsoleInput(
      OnConsoleInputCallbacks::slot_type const& Subscriber )
    {
      return m_CallsOnConsoleInput.connect(Subscriber);
    }

    // Get GUI mutex
    boost::mutex& GuiMgr::GetGuiMutex()
    {
      return m_GuiMutex;
    }

    // Enable watermark
    void GuiMgr::EnableWatermark()
    {
      m_Watermark = true;
    }

    // Disable watermark
    void GuiMgr::DisableWatermark()
    {
      m_Watermark = false;
    }

    // Get GUI instance
    GUI::GUI* GuiMgr::GetGui()
    {
      return m_pGui;
    }
  }
}
