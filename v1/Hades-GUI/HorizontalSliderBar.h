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

#include "GUI.h"

namespace Hades
{
  namespace GUI
  {
    class HorizontalSliderBar : public Element
    {
      friend class VerticalSliderBar;

      int m_iMinValue, m_iMaxValue, m_iValue, m_iStep;
      bool m_bDragged, m_bShowString;

      typedef std::function<std::string (const char* pszArgs, Element* pElement)> tCallback;
      // 	typedef std::string (__cdecl * tCallback)(const char * pszArgs, Element * pElement);
      tCallback m_pUpdater;

      Colour * pLines, * pString;
      Texture * pSlider;

    public:
      HorizontalSliderBar(class GUI& Gui);
      HorizontalSliderBar(class GUI& Gui, TiXmlElement* pElement);

      void SetSliderElement(TiXmlElement * pElement);

      void Draw();
      void PreDraw();
      void MouseMove(Mouse & pMouse);
      bool KeyEvent(Key Key);

      int GetMinValue() const;
      int GetMaxValue() const;
      int GetValue() const;
      int GetStep() const;

      void SetMinValue(int iMinValue);
      void SetMaxValue(int iMaxValue);
      void SetValue(int iValue);
      void SetStep(int iStep);

      bool GetDragged() const;
      void SetDragged(bool bDragged);

      void SetShowString(bool bShow);
      bool GetShowString() const;

      void UpdateTheme(int iIndex);
    };

    std::string MinValue(const char *, Element * pElement);
    std::string MaxValue(const char *, Element * pElement);
    std::string SliderValue(const char *, Element * pElement);
  }
}
