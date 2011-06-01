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

#include "ListBox.h"

namespace Hades
{
  namespace GUI
  {
    ListBox::ListBox(GUI& Gui, TiXmlElement* pElement)
      : Element(Gui)
    {
      SetElement(pElement);
      m_iMouseOverIndex = -1;

      pSlider = new HelperSlider(Gui, Pos(GetWidth() - HELPERSLIDER_WIDTH, 0), GetHeight());

      for(TiXmlElement * pString = pElement->FirstChildElement("Row"); pString; pString = pString->NextSiblingElement("Row"))
        AddRow(pString->GetText());

      SetThemeElement(m_Gui.GetThemeElement("ListBox"));

      if (!GetThemeElement())
        MessageBoxA(0, "Theme element invalid.", "ListBox", 0);
      else
        SetElementState("Norm");
    }

    void ListBox::Draw()
    {
      Pos Pos = GetParent()->GetAbsPos() + GetRelPos();

      m_Gui.DrawOutlinedBox(Pos.GetX(), Pos.GetY(), GetWidth(), GetHeight(), pInner->GetD3DColor(), pBorder->GetD3DColor());

      int iAddHeight = m_Gui.GetFont().GetStringHeight();
      if (m_vRows.size())
        for(int i = pSlider->GetValue(), iHeight = 0; i < static_cast<int>(m_vRows.size()) && iHeight < GetHeight() - m_Gui.GetFont().GetStringHeight(); i++)
        {
          Colour * pColor = 0;

          if (i == m_iMouseOverIndex && GetMouseOver())
            pColor = pMouseOverString;
          else
            pColor = pString;

          m_Gui.GetFont().DrawString(Pos.GetX() + 3, Pos.GetY() + iHeight, 0, pColor, m_vRows[ i ].c_str(), GetWidth() - HELPERSLIDER_WIDTH);
          iHeight += iAddHeight;
        }

        pSlider->Draw(Pos);
    }

    void ListBox::PreDraw()
    {
      pSlider->PreDraw();
    }

    void ListBox::MouseMove(Mouse & pMouse)
    {
      Pos Pos = GetParent()->GetAbsPos() + GetRelPos();

      SetMouseOver(pMouse.InArea(Pos.GetX(), Pos.GetY(), GetWidth(), GetHeight()));

      m_iMouseOverIndex = -1;
      for(int i = pSlider->GetValue(), iHeight = 0, iStringHeight = m_Gui.GetFont().GetStringHeight(); i < static_cast<int>(m_vRows.size()) || iHeight < GetHeight(); i++)
      {
        if (pMouse.InArea(Pos.GetX(), Pos.GetY() + iHeight, GetWidth() - BUTTON_HEIGHT, iStringHeight))
          m_iMouseOverIndex = i;

        iHeight += iStringHeight;
      }

      pSlider->MouseMove(Pos, pMouse);
    }

    bool ListBox::KeyEvent(Key Key)
    {
      Pos Pos = GetParent()->GetAbsPos() + GetRelPos();

      if (!Key.m_Key)
      {
        if (GetMouseOver())
        {
          if (m_iMouseOverIndex >= 0 && GetCallback() && m_Gui.GetMouse().GetLeftButton())
            GetCallback()(reinterpret_cast<char*>(m_iMouseOverIndex), this);
        }
      }

      bool bRet = true;

      if (GetMouseOver() || (!Key.m_Down && !m_Gui.GetMouse().GetWheel()))
      {
        bRet = pSlider->KeyEvent(Pos, Key);
        //MouseMove(m_Gui.GetMouse());
      }

      return bRet;
    }

    void ListBox::AddRow(std::string sString)
    {
      pSlider->SetMaxValue(m_vRows.size());
      m_vRows.push_back(sString);
    }

    std::string ListBox::GetRow(int iIndex) const
    {
      if (iIndex >= 0 && iIndex < static_cast<int>(m_vRows.size()))
        return m_vRows[ iIndex ];
      return std::string();
    }

    void ListBox::Clear()
    {
      m_vRows.clear();
    }

    void ListBox::UpdateTheme(int iIndex)
    {
      SElementState * pState = GetElementState(iIndex);

      pInner = pState->GetColor("Inner");
      pBorder = pState->GetColor("Border");
      pString = pState->GetColor("String");
      pMouseOverString = pState->GetColor("MouseOverString");
    }
  }
}
