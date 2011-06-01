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

#pragma once

// C++ Standard Library
#include <array>
#include <string>

// TinyXML
#include "TinyXML/tinyxml.h"

// Hades
#include "Fwd.h"
#include "Pos.h"

namespace Hades
{
  namespace GUI
  {
    class Element
    {
    public:
      Element(class GUI& Gui);

      virtual ~Element();

      void SetElement(TiXmlElement* pElement);

      Window* GetParent() const;
      void SetParent(Window* pParent);

      Callback GetCallback() const;
      void SetCallback(Callback MyCallback);

      void SetRelPos(Pos RelPos);
      void SetAbsPos(Pos AbsPos);

      Pos GetRelPos() const;
      Pos GetAbsPos() const;

      int GetWidth() const;
      void SetWidth(int Width);

      int GetHeight() const;
      void SetHeight(int Height);

      bool HasFocus() const;

      void SetString(std::string const& MyString, int Index = 0);
      std::string GetString(bool ReplaceVars = false, int Index = 0);
      std::string GetFormatted(int Index = 0) const;

      bool GetMouseOver() const;
      bool SetMouseOver(bool MouseOver);

      SElement* SetThemeElement(SElement* pThemeElement, int iIndex = 0);
      SElement* GetThemeElement(int iIndex = 0) const;

      void SetElementState(std::string const& sState, int iIndex = 0);
      SElementState * GetElementState(int iIndex = 0) const;

      virtual void Draw();
      virtual void PreDraw();
      virtual void MouseMove(Mouse& pMouse);
      virtual bool KeyEvent(Key Key);

      virtual void UpdateTheme(int iIndex);

    protected:
      class GUI& m_Gui;

    private:
      bool m_MouseOver;

      std::array<std::string, 2> m_RawStr;
      std::array<std::string, 2> m_FormattedStr;

      int m_Width;
      int m_Height;

      Pos m_RelPos;
      Pos m_AbsPos;

      Window* m_pParent;

      Callback m_Callback;

      std::array<SElement*, 3> m_ThemeElements;
      std::array<SElementState*, 3> m_ElementStates;
    };
  }
}
