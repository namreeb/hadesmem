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
    Button::Button(GUI& Gui, TiXmlElement* pElement) 
      : Element(Gui)
    {
      SetElement(pElement);
      SetHeight(BUTTON_HEIGHT);

      SetThemeElement(m_Gui.GetThemeElement("Button"));

      if (!GetThemeElement())
        MessageBoxA(0, "Theme element invalid.", "Button", 0);
      else
        SetElementState("Norm");
    }

    void Button::Draw()
    {
      const Pos Pos = GetParent()->GetAbsPos() + GetRelPos();

      pButton->Draw(Pos, GetWidth(), GetHeight());
      m_Gui.GetFont().DrawString(Pos.GetX() + GetWidth() / 2, Pos.GetY() + GetHeight() / 2, FT_CENTER|FT_VCENTER, pString, GetString().c_str());
    }

    void Button::PreDraw()
    {
      if (!m_tPressed.Running())
        SetElementState(GetMouseOver()?"MouseOver":"Norm");
    }

    void Button::MouseMove(Mouse & pMouse)
    {
      Pos Pos = GetParent()->GetAbsPos() + GetRelPos();

      SetElementState(SetMouseOver(pMouse.InArea(Pos.GetX(), Pos.GetY(), GetWidth(), GetHeight()))?"MouseOver":"Norm");
    }

    bool Button::KeyEvent(Key Key)
    {
      if (!Key.m_Key)
      {
        if (GetMouseOver() && m_Gui.GetMouse().GetLeftButton(0))
        {
          SetElementState("Pressed");

          if (GetCallback())
            GetCallback()(0, this);

          m_tPressed.Start(0.1f);
        }
      }

      return true;
    }

    void Button::UpdateTheme(int iIndex)
    {
      SElementState * pState = GetElementState(iIndex);

      pButton = pState->GetTexture("Button");
      pString = pState->GetColor("String");
    }
  }
}
