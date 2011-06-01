/*
Copyright (c) 2010 Jan Miguel Garcia (bobbysing)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "GUI.h"

namespace Hades
{
  namespace GUI
  {
    // Constructor
    GUI::GUI(IDirect3DDevice9* pDevice) 
      : m_Visible(false), 
      m_pMouse(), 
      m_pKeyboard(), 
      m_pFont(), 
      m_pDevice(pDevice), 
      m_pSprite(), 
      m_pLine(), 
      m_PreDrawTimer(), 
      m_Windows(), 
      m_CurTheme(), 
      m_Themes(), 
      m_Callbacks()
    {
      // Ensure device is valid
      if (!m_pDevice)
      {
        BOOST_THROW_EXCEPTION(HadesGuiError() << 
          ErrorFunction("GUI::GUI") << 
          ErrorString("Invalid device."));
      }

      // Create D3DX sprite
      HRESULT SpriteResult = D3DXCreateSprite(pDevice, &m_pSprite);
      if (FAILED(SpriteResult))
      {
        BOOST_THROW_EXCEPTION(HadesGuiError() << 
          ErrorFunction("GUI::GUI") << 
          ErrorString("Could not create sprite.") << 
          ErrorCodeWin(SpriteResult));
      }

      // Create D3DX line
      HRESULT LineResult = D3DXCreateLine(pDevice, &m_pLine);
      if (FAILED(LineResult))
      {
        BOOST_THROW_EXCEPTION(HadesGuiError() << 
          ErrorFunction("GUI::GUI") << 
          ErrorString("Could not create line.") << 
          ErrorCodeWin(LineResult));
      }

      // Create mouse manager
      m_pMouse.reset(new Mouse(*this, pDevice));

      // Create keyboard manager
      m_pKeyboard.reset(new Keyboard(*this));

      // Add default callbacks
      AddCallback("Value", SliderValue);
      AddCallback("MaxValue", MaxValue);
      AddCallback("MinValue", MinValue);
    }

    // Destructor
    GUI::~GUI()
    {
      // Delete all windows
      std::for_each(m_Windows.begin(), m_Windows.end(), 
        [] (Window* pWindow)
      {
        delete pWindow;
      });
    }

    // Load interface data from file
    void GUI::LoadInterfaceFromFile(std::string const& Path)
    {
      // Create XML doc
      TiXmlDocument Document;
      
      // Load interface file into doc
      if (!Document.LoadFile(Path.c_str()))
      {
        std::stringstream ErrorMsg;
        ErrorMsg << "XML error \"" << Document.ErrorDesc() << "\".";

        BOOST_THROW_EXCEPTION(HadesGuiError() << 
          ErrorFunction("GUI::LoadInterfaceFromFile") << 
          ErrorString(ErrorMsg.str()));
      }

      // Get handle to doc
      TiXmlHandle hDoc(&Document);

      // Ensure file format is valid
      TiXmlElement* pGUI = hDoc.FirstChildElement("GUI").Element();
      if (!pGUI)
      {
        return;
      }

      // Set font if specified
      if (TiXmlElement* pFontElement = pGUI->FirstChildElement("Font"))
      {
        int FontSize = 0;
        pFontElement->QueryIntAttribute("size", &FontSize);

        char const* FontFace = pFontElement->Attribute("face");

        m_pFont.reset(new Font(*this, GetDevice(), FontSize, FontFace));
      }

      // Initialize theme data if specified
      if (TiXmlElement* pColorThemes = pGUI->FirstChildElement("ColorThemes"))
      {
        // Set default theme if specified
        const char* pszDefaultTheme = pColorThemes->Attribute("default");
        if (pszDefaultTheme)
        {
          m_CurTheme = pszDefaultTheme;
        }

        // Loop over all themes
        for(TiXmlElement* pThemeElem = pColorThemes->FirstChildElement(); 
          pThemeElem; pThemeElem = pThemeElem->NextSiblingElement())
        {
          // Loop over all theme elems
          for(TiXmlElement* pElemElem = pThemeElem->FirstChildElement(); 
            pElemElem; pElemElem = pElemElem->NextSiblingElement())
          {
            // Create new theme elem
            SElement* CurElem = new SElement();

            // Set default state if specified
            const char* pszDefault = pElemElem->Attribute("default");
            if (pszDefault)
            {
              CurElem->sDefaultState = pszDefault;
            }

            // Loop over all theme elem states
            for(TiXmlElement* pStateElem = pElemElem->
              FirstChildElement("State"); pStateElem; pStateElem = 
              pStateElem->NextSiblingElement("State"))
            {
              // Get state name
              const char * pszString = pStateElem->Attribute("string");
              if (!pszString)
              {
                continue;
              }

              // Create new state
              SElementState* pState = CurElem->m_States[pszString] = 
                new SElementState();

              // Set state parent to current theme
              pState->pParent = CurElem;

              // Loop over all state colours
              for(TiXmlElement* pColourElem = pStateElem->
                FirstChildElement("Color"); pColourElem; pColourElem = 
                pColourElem->NextSiblingElement("Color"))
              {
                // Get name of target element
                pszString = pColourElem->Attribute("string");
                if (!pszString)
                {
                  continue;
                }

                // Create colour for element
                pState->m_Colours[pszString] = new Colour(pColourElem);
              }

              // Loop over all state textures
              for(TiXmlElement* pTexElem = pStateElem->
                FirstChildElement("Texture"); pTexElem; pTexElem = 
                pTexElem->NextSiblingElement("Texture"))
              {
                // Generate path to texture
                std::stringstream TexPath;
                TexPath << pThemeElem->Value() << "/" << pTexElem->
                  Attribute("path");

                // Create texture for element
                Texture* pTexture = pState->m_Textures[pTexElem->
                  Attribute("string")] = new Texture(m_pSprite, 
                  TexPath.str().c_str());

                // Set texture alpha if specified
                int TexAlpha = 0;
                if (pTexElem->QueryIntAttribute("alpha", &TexAlpha) == 
                  TIXML_SUCCESS)
                {
                  pTexture->SetAlpha(static_cast<BYTE>(TexAlpha));
                }
              }

              // Store theme elem
              m_Themes[pThemeElem->Value()][pElemElem->Value()] = CurElem;
            }
          }
        }
      }

      // Create windows if specified
      if (TiXmlElement* pWindows = pGUI->FirstChildElement("Windows"))
      {
        for(TiXmlElement* pWindowElem = pWindows->FirstChildElement(); 
          pWindowElem; pWindowElem = pWindowElem->NextSiblingElement())
        {
          AddWindow(new Window(*this, pWindowElem));
        }
      }
    }

    // Fill area on screen
    void GUI::FillArea(int X, int Y, int Width, int Height, D3DCOLOR MyColour)
    {
      DrawLine(X + Width / 2, Y, X + Width / 2, Y + Height, Width, MyColour);
    }

    // Draw line on screen
    void GUI::DrawLine(int StartX, int StartY, int EndX, int EndY, int Width, 
      D3DCOLOR D3DColour)
    {
      m_pLine->SetWidth(static_cast<float>(Width));

      D3DXVECTOR2 MyVec[2];
      MyVec[0] = D3DXVECTOR2(static_cast<float>(StartX), 
        static_cast<float>(StartY));
      MyVec[1] = D3DXVECTOR2(static_cast<float>(EndX), 
        static_cast<float>(EndY));

      m_pLine->Begin();
      m_pLine->Draw(MyVec, 2, D3DColour);
      m_pLine->End();
    }

    // Draw outlined box on screen
    void GUI::DrawOutlinedBox(int X, int Y, int Width, int Height, 
      D3DCOLOR InnerColour, D3DCOLOR BorderColour)
    {
      FillArea(X + 1, Y + 1, Width - 2, Height - 2, InnerColour);

      DrawLine(X, Y, X, Y + Height, 1, BorderColour);
      DrawLine(X + 1, Y, X + Width - 1,	Y, 1, BorderColour);
      DrawLine(X + 1, Y + Height - 1,	X + Width - 1, Y + Height - 1, 1, 
        BorderColour);
      DrawLine(X + Width - 1,	Y, X + Width - 1,	Y + Height, 1, BorderColour);
    }

    // Add new window
    Window* GUI::AddWindow(Window* pWindow) 
    {
      m_Windows.push_back(pWindow);
      return pWindow;
    }

    // Bring window to top
    void GUI::BringToTop(Window* pWindow)
    {
      m_Windows.erase(std::remove(m_Windows.begin(), m_Windows.end(), 
        pWindow), m_Windows.end());
      m_Windows.push_back(pWindow);
    }

    // Draw
    void GUI::Draw()
    {
      if (!m_Visible)
      {
        return;
      }

      PreDraw();

      std::for_each(m_Windows.begin(), m_Windows.end(), 
        [] (Window* pWindow)
      {
        if (pWindow->IsVisible())
        {
          pWindow->Draw();
        }
      });

      GetMouse().Draw();
    }

    // Pre-draw
    void GUI::PreDraw()
    {
      if (!m_PreDrawTimer.Running())
      {
        std::for_each(m_Windows.rbegin(), m_Windows.rend(), 
          [] (Window* pWindow)
        {
          if (pWindow->IsVisible())
          {
            pWindow->PreDraw();
          }
        });

        m_PreDrawTimer.Start(0.1f);
      }
    }

    // Notify of mouse movement
    void GUI::MouseMove(Mouse& MyMouse)
    {
      if (Element* pDragging = GetMouse().GetDragging())
      {
        pDragging->MouseMove(MyMouse);
        return;
      }

      bool GotWindow = false;

      std::for_each(m_Windows.rbegin(), m_Windows.rend(), 
        [&] (Window* pWindow)
      {
        if (!pWindow->IsVisible())
        {
          return;
        }

        int Height = 0;
        if (!pWindow->GetMaximized())
        {
          Height = TITLEBAR_HEIGHT;
        }

        if (!GotWindow && GetMouse().InArea(pWindow, Height))
        {
          pWindow->MouseMove(MyMouse);
          GotWindow = true;
        }
        else
        {
          MyMouse.SavePos();
          MyMouse.SetPos(-1, -1);
          pWindow->MouseMove(MyMouse);
          MyMouse.LoadPos();
        }
      });
    }

    // Notify of key event
    bool GUI::KeyEvent(Key MyKey)
    {
      bool Top = false;

      if (!MyKey.m_Key && (MyKey.m_Down || (m_pMouse->GetWheel() && 
        !MyKey.m_Down)))
      {
        std::vector<Window*> Repeat;

        // Note: Cannot use iterators as Window::KeyEvent calls 
        // GUI::BringToTop which in turn modifies the container
        for (std::size_t i = m_Windows.size(); i && i--; )
        {
          Window* pWindow = m_Windows[i];

          if (!pWindow->IsVisible())
          {
            continue;
          }

          if (!Top)
          {
            int Height = 0;
            if (!pWindow->GetMaximized())
            {
              Height = TITLEBAR_HEIGHT;
            }

            if (m_pMouse->InArea(pWindow, Height) && !Top)
            {
              pWindow->KeyEvent(MyKey);
              Top = true;
            }
            else
            {
              Repeat.push_back(pWindow);
            }
          }
          else
          {
            m_pMouse->SavePos();
            m_pMouse->SetPos(Pos(-1, -1));
            pWindow->KeyEvent(MyKey);
            m_pMouse->LoadPos();
          }
        }

        std::for_each(Repeat.begin(), Repeat.end(), 
          [&] (Window* pRepeat)
        {
          m_pMouse->SavePos();
          m_pMouse->SetPos(Pos(-1, -1));
          pRepeat->KeyEvent(MyKey);
          m_pMouse->LoadPos();
        });
      }
      else
      {
        Top = false;

        std::for_each(m_Windows.rbegin(), m_Windows.rend(), 
          [&] (Window* pWindow)
        {
          if (pWindow->IsVisible())
          {
            if (pWindow->GetFocussedElement() && pWindow->GetMaximized())
            {
              Top = true;
            }

            pWindow->KeyEvent(MyKey);
          }

          if (!MyKey.m_Down)
          {
            Top = false;
          }
        });
      }

      return Top;
    }

    // Notify of lost device
    void GUI::OnLostDevice()
    {
      m_pDevice = nullptr;

      m_pFont->OnLostDevice();

      m_pSprite->OnLostDevice();

      m_pLine->OnLostDevice();
    }

    // Notify of reset device
    void GUI::OnResetDevice(IDirect3DDevice9* pDevice)
    {
      m_pDevice = pDevice;

      m_pFont->OnResetDevice(pDevice);

      m_pSprite->OnResetDevice();

      m_pLine->OnResetDevice();
    }

    // Get mouse
    Mouse& GUI::GetMouse() const
    {
      return *m_pMouse;
    }

    // Get keyboard
    Keyboard& GUI::GetKeyboard() const
    {
      return *m_pKeyboard;
    }

    // Get D3D device
    IDirect3DDevice9* GUI::GetDevice() const
    {
      return m_pDevice;
    }

    // Get font
    Font& GUI::GetFont() const
    {
      return *m_pFont;
    }

    // Get sprite
    CComPtr<ID3DXSprite> GUI::GetSprite() const
    {
      return m_pSprite;
    }

    // Get window by string
    Window* GUI::GetWindowByString(std::string const& MyString, int Index)
    {
      auto Iter = std::find_if(m_Windows.begin(), m_Windows.end(), 
        std::bind(std::equal_to<std::string>(), 
        std::bind(&Window::GetString, std::placeholders::_1, false, Index), 
        MyString));

      return Iter != m_Windows.end() ? *Iter : nullptr;
    }

    // Get theme element by name
    SElement* GUI::GetThemeElement(std::string const& Element) const
    {
      auto Iter = m_Themes.find(m_CurTheme);
      if (Iter == m_Themes.end())
      {
        return nullptr;
      }

      auto IterInner = Iter->second.find(Element);
      return IterInner != Iter->second.end() ? IterInner->second : nullptr;
    }

    // Set visibility
    void GUI::SetVisible(bool Visible)
    {
      m_Visible = Visible;
    }

    // Get visibility
    bool GUI::IsVisible() const
    {
      return m_Visible;
    }

    // Get callback by name
    Callback GUI::GetCallback(std::string const& Name) const
    {
      auto Iter = m_Callbacks.find(Name);
      return Iter != m_Callbacks.end() ? Iter->second : nullptr;
    }

    // Add callback
    void GUI::AddCallback(std::string const& Name, Callback MyCallback)
    {
      m_Callbacks[Name] = MyCallback;
    }

    // Get all callbacks
    std::map<std::string, Callback> const& GUI::GetCallbackMap() const
    {
      return m_Callbacks;
    }
  }
}
