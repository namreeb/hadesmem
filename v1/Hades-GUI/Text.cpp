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
    Text::Text(GUI& Gui, TiXmlElement* pElement)
      : Element(Gui)
    {
      SetElement(pElement);

      SetThemeElement(m_Gui.GetThemeElement("Text"));

      if (!GetThemeElement())
        MessageBoxA(0, "Theme element invalid.", "Text", 0);
      else
        SetElementState("Norm");
    }

    void Text::Draw()
    {
      Pos Pos = GetParent()->GetAbsPos() + GetRelPos();

      m_Gui.GetFont().DrawString(Pos.GetX(), Pos.GetY(), 0, pString, GetFormatted(), GetWidth());
    }

    void Text::PreDraw()
    {
      GetString(true);
    }

    void Text::MouseMove(Mouse & pMouse)
    {
      Pos Pos = GetParent()->GetAbsPos() + GetRelPos();

      SetElementState(SetMouseOver(pMouse.InArea(Pos.GetX(), Pos.GetY(), m_Gui.GetFont().GetStringWidth(GetFormatted().c_str()), m_Gui.GetFont().GetStringHeight()))?"MouseOver":"Norm");
    }

    void Text::UpdateTheme(int iIndex)
    {
      SElementState * pState = GetElementState(iIndex);

      pString = pState->GetColor("String");
    }
  }
}
