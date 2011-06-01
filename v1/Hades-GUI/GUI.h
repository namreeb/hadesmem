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

#define TITLEBAR_HEIGHT 24
#define BUTTON_HEIGHT	20
#define HELPERSLIDER_WIDTH 20

#define FCR_NONE	0x0
#define FCR_BOLD 	0x1
#define FCR_ITALICS 0x2

#define FT_NONE		0x0
#define FT_CENTER	0x1
#define FT_BORDER	0x2
#define FT_VCENTER	0x4
#define FT_SINGLELINE 0x8

#define D3DFVF_BITMAPFONT	(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define D3DFVF_PRIMITIVES	(D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

// Windows API
#include <Windows.h>
#include <Shlwapi.h>
#include <atlbase.h>
#include <WindowsX.h>

// C++ Standard Library
#include <map>
#include <set>
#include <limits>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <functional>

// Hades-GUI
#include "Fwd.h"
#include "D3D9.h"
#include "TinyXML\tinyxml.h"
#include "Texture.h"
#include "Font.h"
#include "Timer.h"
#include "Pos.h"
#include "Colour.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "Element.h"
#include "Window.h"
#include "HorizontalSliderBar.h"
#include "VerticalSliderBar.h"
#include "HelperSlider.h"
#include "Button.h"
#include "CheckBox.h"
#include "ProgressBar.h"
#include "Text.h"
#include "EditBox.h"
#include "DropDown.h"
#include "TextBox.h"
#include "ListBox.h"
#include "Error.h"

namespace Hades
{
  namespace GUI
  {
    class GUI
    {
    public:
      GUI(IDirect3DDevice9* pDevice);

      ~GUI();

      void LoadInterfaceFromFile(std::string const& Path);

      void FillArea(int X, int Y, int Width, int Height, D3DCOLOR MyColour);

      void DrawLine(int StartX, int StartY, int EndX, int EndY, int Width, D3DCOLOR Colour);

      void DrawOutlinedBox(int X, int Y, int Width, int Height, D3DCOLOR InnerColour, D3DCOLOR BorderColour);

      Window* AddWindow(Window* pWindow);

      void BringToTop(Window* pWindow);

      void Draw();

      void PreDraw();

      void MouseMove(Mouse& MyMouse);

      bool KeyEvent(Key MyKey);

      void OnLostDevice();

      void OnResetDevice(IDirect3DDevice9* pDevice);

      Window* GetWindowByString(std::string const& String, int Index = 0);

      SElement* GetThemeElement(std::string const& Element) const;

      void SetVisible(bool Visible);

      bool IsVisible() const;

      bool ShouldReload() const;

      void Reload();

      Callback GetCallback(std::string const& Name) const;

      void AddCallback(std::string const& Name, Callback MyCallback);

      std::map<std::string, Callback> const& GetCallbackMap() const;

      Mouse& GetMouse() const;

      Keyboard& GetKeyboard() const;

      Font& GetFont() const;

      IDirect3DDevice9* GetDevice() const;

      CComPtr<ID3DXSprite> GetSprite() const;

    private:
      bool m_Visible;

      std::shared_ptr<Mouse> m_pMouse;

      std::shared_ptr<Keyboard> m_pKeyboard;

      std::shared_ptr<Font> m_pFont;

      IDirect3DDevice9* m_pDevice;

      CComPtr<ID3DXSprite> m_pSprite;

      CComPtr<ID3DXLine> m_pLine;

      Timer m_PreDrawTimer;

      std::vector<Window*> m_Windows;

      std::string m_CurTheme;

      std::map<std::string, std::map<std::string, SElement*>> m_Themes;

      std::map<std::string, Callback> m_Callbacks;
    };
  }
}
