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
    Colour::Colour() 
      : m_D3DColour(0)
    { }

    Colour::Colour(int Red, int Green, int Blue, int Alpha)
      : m_D3DColour(0)
    {
      SetD3DColor(D3DCOLOR_RGBA(Red, Green, Blue, Alpha));
    }

    Colour::Colour(D3DCOLOR D3DColour)
      : m_D3DColour(0)
    {
      SetD3DColor(D3DColour);
    }

    Colour::Colour(TiXmlElement* pElement)
      : m_D3DColour(0)
    {
      int Colours[4] = { 0 };

      pElement->QueryIntAttribute("r", &Colours[0]);
      pElement->QueryIntAttribute("g", &Colours[1]);
      pElement->QueryIntAttribute("b", &Colours[2]);
      pElement->QueryIntAttribute("a", &Colours[3]);

      SetD3DColor(D3DCOLOR_RGBA(Colours[0], Colours[1], Colours[2], 
        Colours[3]));
    }

    const Colour Colour::operator / (const int Divisor) const
    {
      return Colour(GetRed() / Divisor, GetGreen() / Divisor, 
        GetBlue() / Divisor, GetAlpha());
    }

    const Colour Colour::operator - (Colour const& SubColor) const
    {
      return Colour(GetRed() - SubColor.GetRed(), 
        GetGreen() - SubColor.GetGreen(), GetBlue() - SubColor.GetBlue(), 
        GetAlpha());
    }

    const Colour Colour::operator + (Colour const& AddColor) const
    {
      return Colour(GetRed() + AddColor.GetRed(), 
        GetGreen() + AddColor.GetGreen(), GetBlue() + AddColor.GetBlue(), 
        GetAlpha());
    }

    const Colour Colour::operator * (const int Multiplicator) const
    {
      return Colour(GetRed() * Multiplicator, GetGreen() * Multiplicator, 
        GetBlue() * Multiplicator, GetAlpha());
    }

    void Colour::SetD3DColor(D3DCOLOR D3DColour)
    {
      m_D3DColour = D3DColour;
    }

    void Colour::SetRed(int Red)
    {
      SetD3DColor(D3DCOLOR_RGBA(Red, GetGreen(), GetBlue(), GetAlpha()));
    }

    void Colour::SetGreen(int Green)
    {
      SetD3DColor(D3DCOLOR_RGBA(GetRed(), Green, GetBlue(), GetAlpha()));
    }

    void Colour::SetBlue(int Blue)
    {
      SetD3DColor(D3DCOLOR_RGBA(GetRed(), GetGreen(), Blue, GetAlpha()));
    }

    void Colour::SetAlpha(int Alpha)
    {
      SetD3DColor(D3DCOLOR_RGBA(GetRed(), GetGreen(), GetBlue(), Alpha));
    }

    D3DCOLOR Colour::GetD3DColor() const
    {
      return m_D3DColour;
    }

    int Colour::GetRed() const
    {
      return (GetD3DColor() >> 16) & 0xFF;
    }

    int Colour::GetGreen() const
    {
      return (GetD3DColor() >> 8) & 0xFF;
    }

    int Colour::GetBlue() const
    {
      return GetD3DColor() & 0xFF;
    }

    int Colour::GetAlpha() const
    {
      return GetD3DColor() >> 24;
    }

    Colour* SElementState::GetColor(std::string const& Name) const
    {
      auto Iter = m_Colours.find(Name);

      if (Iter == m_Colours.end())
      {
        Iter = pParent->m_States.find(pParent->sDefaultState)->second->
          m_Colours.find(Name);

        if (Iter == pParent->m_States.find(pParent->sDefaultState)->second->
          m_Colours.end())
        {
          MessageBoxA(0, "Color not found.", Name.c_str(), 0);
        }
      }

      return Iter->second;
    }

    Texture * SElementState::GetTexture(std::string const& Name) const
    {
      auto Iter = m_Textures.find(Name);

      if (Iter == m_Textures.end())
      {
        Iter = pParent->m_States.find(pParent->sDefaultState)->second->
          m_Textures.find(Name);

        if (Iter == pParent->m_States.find(pParent->sDefaultState)->second->
          m_Textures.end())
        {
          MessageBoxA(0, "Texture not found.", Name.c_str(), 0);
        }
      }

      return Iter->second;
    }
  }
}
