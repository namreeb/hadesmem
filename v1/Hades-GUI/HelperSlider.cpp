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
    void HelperSlider::SetDragged(bool bDragged)
    {
      m_bDragged = bDragged;
    }

    bool HelperSlider::GetDragged()
    {
      return m_bDragged;
    }

    HelperSlider::HelperSlider(GUI& Gui, Pos relPos, int iHeight)
      : HorizontalSliderBar(Gui)
    {
      SetRelPos(relPos);
      SetHeight(iHeight);

      m_bMouseOver[ 0 ] = m_bMouseOver[ 1 ] = m_bMouseOver[ 2 ] = false;
      SetDragged(false);

      SetThemeElement(m_Gui.GetThemeElement("HelperSlider"));
      SetThemeElement(m_Gui.GetThemeElement("HelperSlider"), 2);

      if (!GetThemeElement())
        MessageBoxA(0, "Theme element invalid.", "HelperSlider", 0);
      else
      {
        SetElementState("Norm");
        SetElementState("Norm", 2);
      }

      SetThemeElement(m_Gui.GetThemeElement("HorizontalSliderBar"), 1);

      if (!GetThemeElement(1))
        MessageBoxA(0, "Theme element invalid.", "HorizontalSliderBar", 0);
      else
        SetElementState("Norm", 1);
    }

    void HelperSlider::Draw(Pos basePos)
    {
      Pos MyPos = basePos + GetRelPos();

      m_Gui.DrawOutlinedBox(MyPos.GetX(), MyPos.GetY(), HELPERSLIDER_WIDTH, GetHeight(), pInner->GetD3DColor(), pBorder->GetD3DColor());

      pUpArrow->Draw(Pos(MyPos.GetX() + 1, MyPos.GetY() + 1), HELPERSLIDER_WIDTH - 2, HELPERSLIDER_WIDTH - 2);

      pDownArrow->Draw(Pos(MyPos.GetX() + 1, MyPos.GetY() + 1 + GetHeight() - HELPERSLIDER_WIDTH), HELPERSLIDER_WIDTH - 2, HELPERSLIDER_WIDTH - 2);

      if (GetMaxValue() < 2)
        return;

      pSlider->Draw(Pos(MyPos.GetX() + 1, MyPos.GetY() - 1 + HELPERSLIDER_WIDTH + static_cast<int>(floor(static_cast<float>((GetHeight() - ((HELPERSLIDER_WIDTH - 2) * 3))) / GetMaxValue() * GetValue()))), HELPERSLIDER_WIDTH - 2, HELPERSLIDER_WIDTH - 2);
    }

    void HelperSlider::PreDraw()
    {
      if (m_bPressed[ 0 ] && !m_tUpArrow.Running())
      {
        m_bPressed[ 0 ] = false;
        SetElementState(m_bMouseOver[ 0 ]?"MouseOver":"Norm");
      }
      if (m_bPressed[ 1 ] && !m_tDownArrow.Running())
      {
        m_bPressed[ 1 ] = false;
        SetElementState(m_bMouseOver[ 1 ]?"MouseOver":"Norm", 2);
      }
    }

    void HelperSlider::MouseMove(Pos basePos, Mouse & pMouse)
    {
      Pos MyPos = basePos + GetRelPos();

      SetElementState(((m_bMouseOver[ 0 ]	= pMouse.InArea(MyPos.GetX() + 1, MyPos.GetY() + 1, HELPERSLIDER_WIDTH - 2, HELPERSLIDER_WIDTH - 2)) != 0)?"MouseOver":"Norm");
      SetElementState(((m_bMouseOver[ 1 ]	= pMouse.InArea(MyPos.GetX() + 1, MyPos.GetY() + 1 + GetHeight() - HELPERSLIDER_WIDTH, HELPERSLIDER_WIDTH - 2, HELPERSLIDER_WIDTH - 2)) != 0)?"MouseOver":"Norm", 2);

      if (GetMaxValue() < 2)
        return;

      if (GetDragged())
      {
        Pos mPos = pMouse.GetPos();

        if (mPos.GetX() == -1 && mPos.GetY() == -1)
          mPos = pMouse.GetSavedPos();

        if (mPos.GetY() < MyPos.GetY() + HELPERSLIDER_WIDTH)
          SetValue(GetMinValue());
        else if (mPos.GetY() > MyPos.GetY() + GetHeight() - HELPERSLIDER_WIDTH)
          SetValue(GetMaxValue());
        else
          for(int i = GetMinValue(); i <= GetMaxValue(); i++)
          {
            int iY = MyPos.GetY() - 1 + HELPERSLIDER_WIDTH + static_cast<int>(floor(static_cast<float>((GetHeight() - ((HELPERSLIDER_WIDTH - 2) * 3))) / GetMaxValue() * i));
            if (mPos.GetY() >= iY && mPos.GetY() <= iY + HELPERSLIDER_WIDTH / 2)
            {
              SetValue(i);
              break;
            }
          }
      }
      else
        SetElementState(((m_bMouseOver[ 2 ]	= pMouse.InArea(MyPos.GetX(),  MyPos.GetY() + HELPERSLIDER_WIDTH + static_cast<int>(floor(static_cast<float>((GetHeight() - (HELPERSLIDER_WIDTH * 3))) / GetMaxValue() * GetValue())), HELPERSLIDER_WIDTH, HELPERSLIDER_WIDTH)) != 0)?"MouseOver":"Norm", 1);
    }

    bool HelperSlider::KeyEvent(Pos basePos, Key Key)
    {
      Pos Pos = basePos + GetRelPos();

      if (!Key.m_Key)
      {
        if (m_Gui.GetMouse().GetLeftButton())
        {
          if (m_bMouseOver[ 0 ])
          {
            if (GetValue() > GetMinValue())
              SetValue(GetValue() - 1);

            m_bPressed[ 0 ] = true;
            SetElementState("Pressed");

            m_tUpArrow.Start(0.1f);
          }
          else if (m_bMouseOver[ 1 ])
          {
            if (GetValue() < GetMaxValue())
              SetValue(GetValue() + 1);

            m_bPressed[ 1 ] = true;
            SetElementState("Pressed", 2);

            m_tDownArrow.Start(0.1f);
          }
          else if (m_Gui.GetMouse().InArea(Pos.GetX() + 1, Pos.GetY() + HELPERSLIDER_WIDTH, HELPERSLIDER_WIDTH, GetHeight() - HELPERSLIDER_WIDTH * 2))
          {	
            SetElementState("Pressed", 1);

            SetDragged(true);
            MouseMove(Pos, m_Gui.GetMouse());
          }
        }
        else
        {
          SetElementState(m_bMouseOver[ 2 ]?"MouseOver":"Norm", 1);
          SetDragged(false);
        }

        if (!GetDragged() && m_Gui.GetMouse().GetWheel())
        {
          int iState = m_Gui.GetMouse().GetWheel(0);

          int iTenth = (GetMaxValue() / 10)?GetMaxValue() / 10:1;

          if (iState == 1)
            SetValue(GetValue() - iTenth);
          else if (iState == 2)
            SetValue(GetValue() + iTenth);	
        }
      }
      return true;
    }

    void HelperSlider::UpdateTheme(int iIndex)
    {
      SElementState * pState = GetElementState(iIndex);

      if (!iIndex)
      {
        pInner = pState->GetColor("Inner");
        pBorder = pState->GetColor("Border");

        pUpArrow = pState->GetTexture("UpArrow");
      }
      else if (iIndex == 1)
        pSlider = pState->GetTexture("Slider");
      else
        pDownArrow = pState->GetTexture("DownArrow");
    }
  }
}
