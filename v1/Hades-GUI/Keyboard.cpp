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
#include "GUI.h"
#include "Mouse.h"
#include "Keyboard.h"

namespace Hades
{
  namespace GUI
  {
    Keyboard::Keyboard(GUI& Gui) 
      : m_Gui(Gui), 
      m_Key()
    { }

    bool Keyboard::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      if (!m_Gui.IsVisible() || uMsg < WM_KEYFIRST || uMsg > WM_KEYLAST || 
        m_Gui.GetMouse().GetLeftButton())
      {
        return false;
      }

      switch(uMsg)
      {
      case WM_KEYDOWN:
        SetKey(Key(static_cast<char>(wParam), true, lParam));
        break;
      case WM_KEYUP:
        SetKey(Key(static_cast<char>(wParam), false, lParam));
        break;
      }

      return m_Gui.KeyEvent(GetKey());
    }

    void Keyboard::SetKey(Key Key)
    {
      m_Key = Key;
    }

    Key Keyboard::GetKey()
    {
      Key sRet = m_Key;
      SetKey(Key(0, false));
      return sRet;
    }
  }
}
