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
#include <memory>

// DirectX
#include <d3d9.h>

// Hades
#include "Pos.h"
#include "Timer.h"

namespace Hades
{
  namespace GUI
  {
    class Mouse
    {
    public:
      Mouse(class GUI& Gui, IDirect3DDevice9* pDevice);

      bool HandleMessage(unsigned int uMsg, WPARAM wParam, LPARAM lParam);

      void SetPos(int X, int Y);
      void SetPos(Pos MyPos);
      Pos GetPos() const;

      bool InArea(int X, int Y, int Width, int Height) const;
      bool InArea(class Element* pElement, int Height = 0) const;

      void Draw();

      int GetLeftButton(int State = -1);
      int GetRightButton(int State = -1);
      int GetMiddleButton(int State = -1);
      int GetWheel(int State = -1);

      void SetLeftButton(int State);
      void SetRightButton(int State);
      void SetMiddleButton(int State);
      void SetWheel(int State);

      void SetDragging(class Element* pElement);
      class Element* GetDragging() const;

      void SavePos();
      void LoadPos();
      Pos GetSavedPos() const;

    private:
      class GUI& m_Gui;
      IDirect3DDevice9* m_pDevice;
      Pos m_Pos, m_BakPos;
      std::shared_ptr<class Colour> m_pInnerColor;
      std::shared_ptr<class Colour> m_pBorderColor;
      class Element* m_pDraggingElement;
      int m_LeftButtonState, m_RightButtonState, m_MiddleButtonState, 
        m_WheelState;
      Timer m_LeftButtonTimer, m_RightButtonTimer, m_MiddleButtonTimer;
    };
  }
}
