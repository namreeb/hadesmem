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

// Hades
#include "Element.h"

#include "GUI.h"

namespace Hades
{
  namespace GUI
  {
    Element::Element(GUI& Gui) 
      : m_Gui(Gui), 
      m_MouseOver(false), 
      m_RawStr(), 
      m_FormattedStr(), 
      m_Width(0), 
      m_Height(0), 
      m_RelPos(), 
      m_AbsPos(), 
      m_pParent(nullptr), 
      m_Callback(), 
      m_ThemeElements(), 
      m_ElementStates()
    {
      m_RawStr.assign(std::string());
      m_FormattedStr.assign(std::string());

      m_ThemeElements.assign(nullptr);
      m_ElementStates.assign(nullptr);
    }

    Element::~Element()
    { }

    void Element::SetElement(TiXmlElement* pElement)
    {
      if (!pElement)
      {
        return;
      }

      SetMouseOver(false);

      int TempX = 0, TempY = 0;

      if (pElement->QueryIntAttribute("relX", &TempX) == TIXML_NO_ATTRIBUTE)
      {
        TempX = 0;
      }
      if (pElement->QueryIntAttribute("relY", &TempY) == TIXML_NO_ATTRIBUTE)
      {
        TempY = 0;
      }

      SetRelPos(Pos(TempX, TempY));

      if (pElement->QueryIntAttribute("absX", &TempX) == TIXML_NO_ATTRIBUTE)
      {
        TempX = 0;
      }
      if (pElement->QueryIntAttribute("absY", &TempY) == TIXML_NO_ATTRIBUTE)
      {
        TempY = 0;
      }

      SetAbsPos(Pos(TempX, TempY));

      if (pElement->QueryIntAttribute("width", &TempX) == TIXML_NO_ATTRIBUTE)
      {
        TempX = 0;
      }
      if (pElement->QueryIntAttribute("height", &TempY) == TIXML_NO_ATTRIBUTE)
      {
        TempY = 0;
      }

      SetWidth(TempX);
      SetHeight(TempY);

      if (const char* pszString = pElement->Attribute("string"))
      {
        SetString(pszString);
      }

      if (const char* pszString = pElement->Attribute("string2"))
      {
        SetString(pszString, 1);
      }

      if (const char* pszCallback = pElement->Attribute("callback"))
      {
        SetCallback(m_Gui.GetCallback(pszCallback));

        if (!m_Callback)
        {
          BOOST_THROW_EXCEPTION(HadesGuiError() << 
            ErrorFunction("Element::SetElement") << 
            ErrorString("Invalid callback."));
        }
      }
      else
      {
        SetCallback(nullptr);
      }
    }

    void Element::SetParent(Window* pParent)
    {
      m_pParent = pParent;
    }

    Window* Element::GetParent() const
    {
      return m_pParent;
    }

    void Element::SetCallback(Callback MyCallback)
    {
      m_Callback = MyCallback;
    }

    Callback Element::GetCallback() const
    {
      return m_Callback;
    }

    void Element::SetRelPos(Pos RelPos)
    {
      m_RelPos = RelPos;
    }

    Pos Element::GetRelPos() const
    {
      return m_RelPos;
    }

    void Element::SetAbsPos(Pos AbsPos)
    {
      m_AbsPos = AbsPos;
    }

    Pos Element::GetAbsPos() const
    {
      return m_AbsPos;
    }

    void Element::SetWidth(int Width)
    {
      m_Width = Width;
    }

    int Element::GetWidth() const
    {
      return m_Width;
    }

    void Element::SetHeight(int Height)
    {
      m_Height = Height;
    }

    int Element::GetHeight() const
    {
      return m_Height;
    }

    bool Element::HasFocus() const
    {
      return GetParent()->GetFocussedElement() == this;
    }

    void Element::SetString(std::string const& MyString, int Index)
    {
      if (MyString.length() > 255)
      {
        return;
      }

      if (Index >= m_RawStr.size())
      {
        BOOST_THROW_EXCEPTION(HadesGuiError() << 
          ErrorFunction("Element::SetString") << 
          ErrorString("Invalid index."));
      }

      m_RawStr[Index] = MyString;
    }

    std::string Element::GetString(bool ReplaceVars, int Index)
    {
      if (!ReplaceVars)
      {
        return m_RawStr[Index];
      }

      std::size_t StrPos = 0;
      std::string& FormattedStr = m_FormattedStr[Index] = m_RawStr[Index];

      while((StrPos = FormattedStr.find("$", StrPos)) != std::string::npos)
      {
        for(auto Iter = m_Gui.GetCallbackMap().rbegin(); 
          Iter != m_Gui.GetCallbackMap().rend(); ++Iter)
        {
          std::string const& CallbackName = Iter->first;

          if (!FormattedStr.compare(StrPos, CallbackName.length(), 
            CallbackName))
          {
            FormattedStr = FormattedStr.replace(StrPos, CallbackName.length(), 
              Iter->second(0, this));
            break;
          }
        }

        ++StrPos;
      }

      return FormattedStr;
    }

    std::string Element::GetFormatted(int Index) const
    {
      return m_FormattedStr[Index];
    }

    bool Element::GetMouseOver() const
    {
      return m_MouseOver;
    }

    bool Element::SetMouseOver(bool MouseOver)
    {
      return m_MouseOver = MouseOver;
    }

    SElement * Element::SetThemeElement(SElement* pThemeElement, int Index)
    {
      return m_ThemeElements[Index] = pThemeElement;
    }

    SElement * Element::GetThemeElement(int Index) const
    {
      return m_ThemeElements[Index];
    }

    void Element::SetElementState(std::string const& State, int Index)
    {
      m_ElementStates[Index] = GetThemeElement(Index)->m_States[State];

      if (!m_ElementStates.data())
      {
        m_ElementStates[Index] = GetThemeElement(Index)->m_States[
          GetThemeElement(Index)->sDefaultState];
      }

      UpdateTheme(Index);
    }

    SElementState * Element::GetElementState(int Index) const
    {
      return m_ElementStates[ Index ];
    }

    void Element::UpdateTheme(int /*Index*/)
    { }

    void Element::Draw()
    { }

    void Element::PreDraw()
    { }

    void Element::MouseMove(Mouse& /*MyMouse*/)
    { }

    bool Element::KeyEvent(Key /*MyKey*/)
    {
      return true;
    }
  }
}
