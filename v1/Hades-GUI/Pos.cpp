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
#include "Pos.h"

namespace Hades
{
  namespace GUI
  {
    Pos::Pos(Pos* pPos)
      : m_X(pPos->GetX()), 
      m_Y(pPos->GetY())
    { }

    Pos::Pos(int X, int Y)
      : m_X(X), 
      m_Y(Y)
    { }

    Pos::Pos()
      : m_X(0), 
      m_Y(0)
    { }

    int Pos::GetX() const
    {
      return m_X;
    }

    int Pos::GetY() const
    {
      return m_Y;
    }

    void Pos::SetX(int X)
    {
      m_X = X;
    }

    void Pos::SetY(int Y)
    {
      m_Y = Y;
    }

    const Pos Pos::operator + (Pos const& Rhs) const
    {
      return Pos(GetX() + Rhs.GetX(), GetY() + Rhs.GetY());
    }

    const Pos Pos::operator - (Pos const& Rhs) const
    {
      return Pos(GetX() - Rhs.GetX(), GetY() - Rhs.GetY());
    }
  }
}
