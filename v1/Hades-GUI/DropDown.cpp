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
    DropDown::DropDown(GUI& Gui, TiXmlElement* pElement)
      : Element(Gui)
    {
      SetElement(pElement);
      SetHeight(BUTTON_HEIGHT);

      m_bDropped = false;
      m_iSelected = m_iMouseOverIndex = 0;

      for(TiXmlElement * pEntryElement = pElement->FirstChildElement("Entry"); pEntryElement; pEntryElement = pEntryElement->NextSiblingElement("Entry"))
        AddElement(pEntryElement->Attribute("string"), pEntryElement->Attribute("value"));

      SetThemeElement(m_Gui.GetThemeElement("DropDown"));

      if (!GetThemeElement())
        MessageBoxA(0, "Theme element invalid.", "DropDown", 0);
      else
        SetElementState("Norm");
    }

    void DropDown::Draw()
    {
      Pos MyPos = GetParent()->GetAbsPos() + GetRelPos();

      SElementState * pState = GetElementState();

      if (pState)
      {
        m_Gui.DrawOutlinedBox(MyPos.GetX(), MyPos.GetY(), GetWidth(), GetHeight(), pInner->GetD3DColor(), pBorder->GetD3DColor());
        m_Gui.DrawLine(MyPos.GetX() + GetWidth() - 20, MyPos.GetY() + 1, MyPos.GetX() + GetWidth() - 20, MyPos.GetY() + GetHeight() - 1, 1, pBorder->GetD3DColor());

        m_Gui.GetFont().DrawString(MyPos.GetX() + 3, MyPos.GetY() + GetHeight() / 2, FT_VCENTER, pString, m_vEntrys[ m_iSelected ].m_sString.c_str());

        if (m_bDropped && m_vEntrys.size())
        {
          m_Gui.DrawOutlinedBox(MyPos.GetX(), MyPos.GetY() + GetHeight(), GetWidth(), GetHeight() * m_vEntrys.size(), pInner->GetD3DColor(), pBorder->GetD3DColor());

          for(int iIndex = 0; iIndex < static_cast<int>(m_vEntrys.size()); iIndex++)
          {
            if (iIndex == m_iMouseOverIndex)
            {
              m_Gui.FillArea(MyPos.GetX() + 1, MyPos.GetY() + GetHeight() * (iIndex + 1), GetWidth() - 2, GetHeight(),pSelectedInner->GetD3DColor());
              m_Gui.GetFont().DrawString(MyPos.GetX() + 3, MyPos.GetY() + GetHeight() * (iIndex + 1) + GetHeight() / 2, FT_VCENTER, pSelectedString, m_vEntrys[ iIndex ].m_sString.c_str());
            }
            else
              m_Gui.GetFont().DrawString(MyPos.GetX() + 3, MyPos.GetY() + GetHeight() * (iIndex + 1) + GetHeight() / 2, FT_VCENTER, pString, m_vEntrys[ iIndex ].m_sString.c_str());
          }
        }

        pButton->Draw(Pos(MyPos.GetX() + GetWidth() - 19, MyPos.GetY() + 1), 18, 18);
      }
    }

    void DropDown::MouseMove(Mouse & pMouse)
    {
      Pos Pos = GetParent()->GetAbsPos() + GetRelPos(), mPos = m_Gui.GetMouse().GetPos();

      int iHeight = 0;
      if (m_bDropped)
      {
        iHeight = GetHeight() * (m_vEntrys.size() + 1);

        if (mPos.GetX() == -1 && mPos.GetY() == -1)
          pMouse.LoadPos();
      }
      else
        iHeight = GetHeight();

      SetElementState(SetMouseOver(pMouse.InArea(Pos.GetX(), Pos.GetY(), GetWidth(), iHeight))?"MouseOver":"Norm");

      if (GetMouseOver())
        for(int iIndex = 0; iIndex < static_cast<int>(m_vEntrys.size()); iIndex++)
          if (m_Gui.GetMouse().InArea(Pos.GetX(), Pos.GetY() + GetHeight() * (iIndex + 1), GetWidth(), GetHeight()))
          {
            m_iMouseOverIndex = iIndex;
            break;
          }

          pMouse.SetPos(mPos);
    }

    bool DropDown::KeyEvent(Key Key)
    {
      if (!Key.m_Key)
      {
        if (m_Gui.GetMouse().GetLeftButton())
        {
          if (GetMouseOver())
          {
            if (m_bDropped)
            {	
              m_iSelected = m_iMouseOverIndex;
              if (GetCallback())
                GetCallback()(m_vEntrys[ m_iSelected ].m_sValue.c_str(), this);

              m_bDropped = false;
              SetElementState("Norm");
            }
            else
            {
              GetParent()->BringToTop(this);

              m_bDropped = true;
              SetElementState("Pressed");
            }

            m_Gui.GetMouse().SetLeftButton(0);
          }
          else
            m_bDropped = false;
        }
      }

      return !GetMouseOver();
    }

    void DropDown::AddElement(std::string sElem, std::string sValue)
    {
      m_vEntrys.push_back(SEntry(sElem, sValue));
    }

    std::string DropDown::GetValue() const
    {
      return m_vEntrys[ m_iSelected ].m_sValue;
    }

    void DropDown::UpdateTheme(int iIndex)
    {
      SElementState * pState = GetElementState(iIndex);

      pString = pState->GetColor("String");
      pInner = pState->GetColor("Inner");
      pBorder = pState->GetColor("Border");
      pSelectedInner = pState->GetColor("SelectedInner");
      pSelectedString = pState->GetColor("SelectedString");

      pButton = pState->GetTexture("Button");
    }
  }
}
