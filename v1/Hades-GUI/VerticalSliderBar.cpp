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
    VerticalSliderBar::VerticalSliderBar(GUI& Gui, TiXmlElement * pElement)
      : HorizontalSliderBar(Gui)
    {
      SetDragged(false);
      SetCallback(0);
      m_iMinValue = 0, m_iMaxValue = 0, m_iValue = 0;

      SetSliderElement(pElement);

      SetThemeElement(m_Gui.GetThemeElement("VerticalSliderBar"));

      if (!GetThemeElement())
        MessageBoxA(0, "Theme element invalid.", "VerticalSliderBar", 0);
      else
        SetElementState("Norm");
    }

    void VerticalSliderBar::Draw()
    {
      Pos MyPos = GetParent()->GetAbsPos() + GetRelPos();

      D3DCOLOR d3dLineColor = pLines->GetD3DColor();

      m_Gui.DrawLine(MyPos.GetX() + TITLEBAR_HEIGHT / 2, MyPos.GetY(),					MyPos.GetX() + TITLEBAR_HEIGHT / 2,		MyPos.GetY() + GetHeight(), 1,		d3dLineColor);
      m_Gui.DrawLine(MyPos.GetX() + TITLEBAR_HEIGHT / 4, MyPos.GetY(),					MyPos.GetX() + TITLEBAR_HEIGHT / 4 * 3,	MyPos.GetY(), 1,						d3dLineColor);
      m_Gui.DrawLine(MyPos.GetX() + TITLEBAR_HEIGHT / 4, MyPos.GetY() + GetHeight(),		MyPos.GetX() + TITLEBAR_HEIGHT / 4 * 3,	MyPos.GetY() + GetHeight(), 1,		d3dLineColor);
      m_Gui.DrawLine(MyPos.GetX() + TITLEBAR_HEIGHT / 4, MyPos.GetY() + GetHeight() / 2,	MyPos.GetX() + TITLEBAR_HEIGHT / 4 * 3,	MyPos.GetY() + GetHeight() / 2, 1,	d3dLineColor);

      pSlider->Draw(Pos(MyPos.GetX() + 2, MyPos.GetY() + GetHeight() - static_cast<int>(floor(static_cast<float>(GetHeight()) / GetMaxValue() * GetValue())) - 5), BUTTON_HEIGHT, 10);

      m_Gui.GetFont().DrawString(MyPos.GetX() - TITLEBAR_HEIGHT + (TITLEBAR_HEIGHT * 3) / 2, MyPos.GetY() + GetHeight() + 5, FT_CENTER, pString, GetFormatted());
    }

    void VerticalSliderBar::MouseMove(Mouse & pMouse)
    {
      Pos MyPos = GetParent()->GetAbsPos() + GetRelPos();

      if (GetDragged())
      {
        Pos mousePos = pMouse.GetPos();

        if (mousePos.GetX() == -1 && mousePos.GetY() == -1)
          mousePos = pMouse.GetSavedPos();

        if (mousePos.GetY() < MyPos.GetY())
          SetValue(GetMaxValue());
        else if (mousePos.GetY() > MyPos.GetY() + GetHeight())
          SetValue(GetMinValue());
        else
        {
          for(int iIndex = GetMinValue(); iIndex < GetMaxValue(); iIndex++)
            if (mousePos.GetY() >= MyPos.GetY() + floor(static_cast<float>(GetHeight()) / GetMaxValue() * iIndex) && mousePos.GetY() <= MyPos.GetY() + floor((float)GetHeight() / GetMaxValue() * (iIndex + 1)))
            {
              SetValue(GetMaxValue() - iIndex);
              break;
            }
        }

        if (GetCallback())
          GetCallback()(reinterpret_cast<const char*>(GetValue()), this);
      }
      else
        SetElementState(SetMouseOver(m_Gui.GetMouse().InArea(MyPos.GetX(), MyPos.GetY(), TITLEBAR_HEIGHT, GetHeight()))?"MouseOver":"Norm");
    }

    void VerticalSliderBar::UpdateTheme(int iIndex)
    {
      SElementState * pState = GetElementState(iIndex);

      pLines = pState->GetColor("Lines");
      pString = pState->GetColor("String");

      pSlider = pState->GetTexture("Slider");
    }
  }
}
