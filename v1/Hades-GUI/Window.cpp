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
    Window::Window(GUI& Gui, TiXmlElement* pElement)
      : Element(Gui), 
      m_Maximized(true), 
      m_Dragging(false), 
      m_Visible(true), 
      m_PosDiff(), 
      m_Elements(), 
      m_pFocussedElement(nullptr), 
      m_CloseButtonEnabled(true), 
      m_pTitle(nullptr), 
      m_pBodyInner(nullptr), 
      m_pBodyBorder(nullptr), 
      m_pTitlebar(nullptr), 
      m_pButton(nullptr)
    {
      SetElement(pElement);

      if (const char* pszVisible = pElement->Attribute("hidden"))
      {
        SetVisible(pszVisible[ 0 ] != '1');
      }

      LoadElement<Text>(pElement, "Text");
      LoadElement<Button>(pElement, "Button");
      LoadElement<ListBox>(pElement, "ListBox");
      LoadElement<EditBox>(pElement, "EditBox");
      LoadElement<TextBox>(pElement, "TextBox");
      LoadElement<CheckBox>(pElement, "CheckBox");
      LoadElement<DropDown>(pElement, "DropDown");
      LoadElement<ProgressBar>(pElement, "ProgressBar");
      LoadElement<VerticalSliderBar>(pElement, "VerticalSliderBar");
      LoadElement<HorizontalSliderBar>(pElement, "HorizontalSliderBar");

      SetThemeElement(m_Gui.GetThemeElement("Window"));

      if (!GetThemeElement())
      {
        MessageBoxA(0, "Theme element invalid.", "Window", 0);
      }
      else
      {
        SetElementState("Norm");
      }

      SetThemeElement(m_Gui.GetThemeElement("CloseButton"), 1);

      if (!GetThemeElement(1))
      {
        MessageBoxA(0, "Theme element invalid.", "CloseButton", 0);
      }
      else
      {
        SetElementState("Norm", 1);

        if (const char* pszVisible = pElement->Attribute("closebutton"))
        {
          SetCloseButton(pszVisible[ 0 ] == '1');
        }

        MouseMove(m_Gui.GetMouse());
      }
    }

    Window::~Window()
    {
      std::for_each(m_Elements.begin(), m_Elements.end(), 
        [] (Element* pElement) 
      {
        delete pElement;
      });
    }

    void Window::AddElement(Element* pElement)
    {
      pElement->SetRelPos(pElement->GetRelPos() + Pos(0, TITLEBAR_HEIGHT));
      pElement->SetParent(this);

      m_Elements.push_back(pElement);
    }

    void Window::Draw()
    {	
      m_pTitlebar->Draw(GetAbsPos(), GetWidth(), TITLEBAR_HEIGHT);
      m_Gui.GetFont().DrawString(GetAbsPos().GetX() + 5, GetAbsPos().GetY() + 5, 0, m_pTitle, GetFormatted());
      m_pButton->Draw(Pos(GetAbsPos().GetX() + GetWidth() - BUTTON_HEIGHT - 2, GetAbsPos().GetY() + 2), BUTTON_HEIGHT, BUTTON_HEIGHT);

      if (GetMaximized())
      {
        m_Gui.DrawOutlinedBox(GetAbsPos().GetX(), GetAbsPos().GetY() + TITLEBAR_HEIGHT, GetWidth(), GetHeight() - TITLEBAR_HEIGHT + 1,  m_pBodyInner->GetD3DColor(), m_pBodyBorder->GetD3DColor());

        for each(Element * pElement in m_Elements)
          pElement->Draw();
      }
    }

    void Window::PreDraw()
    {
      GetString(true);

      if (GetMaximized())
        for each(Element * pElement in m_Elements)
          pElement->PreDraw();
    }

    void Window::MouseMove(Mouse & pMouse)
    {
      if (GetDragging())
      {
        if (!m_PosDiff.GetX())
          m_PosDiff = GetAbsPos() - pMouse.GetPos();
        else
        {
          Pos mPos = pMouse.GetPos();

          if (mPos.GetX() == -1 && mPos.GetY() == -1)
            mPos = pMouse.GetSavedPos();

          SetAbsPos(mPos + m_PosDiff);
        }
      }

      if (GetCloseButton())
        SetElementState(SetMouseOver(pMouse.InArea(GetAbsPos().GetX() + GetWidth() - BUTTON_HEIGHT - 2, GetAbsPos().GetY() + 2, BUTTON_HEIGHT, BUTTON_HEIGHT))?"MouseOver":"Norm", 1);

      if (GetMaximized())
        for each(Element * pElement in m_Elements)
          pElement->MouseMove(pMouse);
    }

    bool Window::KeyEvent(Key Key)
    {
      Mouse & Mouse = m_Gui.GetMouse();

      if (Mouse.GetLeftButton())
      {
        SetFocussedElement(0);

        if (GetMouseOver() && m_CloseButtonEnabled)
          this->SetVisible(false);
        else if (Mouse.InArea(GetAbsPos().GetX(), GetAbsPos().GetY(), GetWidth(), TITLEBAR_HEIGHT))
        {
          if (!Mouse.GetDragging())
          {
            if (Mouse.GetLeftButton() == 1)
            {
              m_Gui.BringToTop(this);

              SetDragging(true);
              Mouse.SetDragging(this);

              SetElementState("Dragging");
            }
            else
            {
              SetMaximized(!GetMaximized());

              SetElementState(GetMaximized()?"Norm":"Minimized");

              m_Gui.BringToTop(this);
            }
          }
        }
        else if (Mouse.InArea(GetAbsPos().GetX(), GetAbsPos().GetY(), GetWidth(), GetHeight()))
          m_Gui.BringToTop(this);
      }
      else
      {
        m_PosDiff.SetX(0);

        Mouse.SetDragging(0);
        SetDragging(false);

        SetElementState(GetMaximized()?"Norm":"Minimized");
      }

      if (GetMaximized())
        for(int iIndex = static_cast<int>(m_Elements.size()) - 1; iIndex >= 0; iIndex--)
          if (!m_Elements[ iIndex ]->KeyEvent(Key))
            return false;
      return true;
    }

    void Window::SetMaximized(bool Maximized)
    {
      m_Maximized = Maximized;
    }

    bool Window::GetMaximized()
    {
      return m_Maximized;
    }

    void Window::SetVisible(bool Visible)
    {
      m_Visible = Visible;
    }

    bool Window::IsVisible()
    {
      return m_Visible;
    }

    void Window::SetDragging(bool Dragging)
    {
      m_Dragging = Dragging;
    }

    bool Window::GetDragging()
    {
      return m_Dragging;
    }

    void Window::SetCloseButton(bool bEnabled)
    {
      m_CloseButtonEnabled = bEnabled;

      if (GetCloseButton())
      {
        SetElementState("Disabled", 1);
      }
    }

    bool Window::GetCloseButton()
    {
      return m_CloseButtonEnabled;
    }

    void Window::SetFocussedElement(Element* pElement)
    {
      m_pFocussedElement = pElement;

      if (pElement)
      {
        BringToTop(pElement);
      }
    }

    Element* Window::GetFocussedElement()
    {
      return m_pFocussedElement;
    }

    Element* Window::GetElementByString(std::string const& MyString, int Index)
    {
      auto Iter = std::find_if(m_Elements.begin(), m_Elements.end(), 
        [&] (Element* pElement)
      {
        return pElement->GetString(false, Index) == MyString;
      });

      return Iter != m_Elements.end() ? *Iter : nullptr;
    }

    void Window::BringToTop(Element* pElement)
    {
      m_Elements.erase(std::remove(m_Elements.begin(), m_Elements.end(), 
        pElement), m_Elements.end());
      m_Elements.push_back(pElement);
    }

    void Window::UpdateTheme(int Index)
    {
      SElementState* pState = GetElementState(Index);
      if (!Index)
      {
        m_pTitle = pState->GetColor("Title");
        m_pBodyInner = pState->GetColor("BodyInner");
        m_pBodyBorder = pState->GetColor("BodyBorder");

        m_pTitlebar = pState->GetTexture("Titlebar");
      }
      else
      {
        m_pButton = pState->GetTexture("Button");
      }
    }
  }
}
