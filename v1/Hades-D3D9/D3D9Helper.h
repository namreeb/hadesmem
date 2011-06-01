/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// C++ Standard Library
#include <memory>

// Windows API
#include <d3d9.h>
#include <d3dx9.h>
#include <atlbase.h>

// Hades
#include "Hades-Common/VecN.h"

namespace Hades
{
  namespace D3D9
  {
    // D3D9Helper pointer type
    typedef std::tr1::shared_ptr<class D3D9Helper> D3D9HelperPtr;

    // D3D9 helper class
    class D3D9Helper
    {
    public:
      // Constructor
      D3D9Helper()
        : m_pLine(nullptr) 
      { }

      // OnLostDevice callback
      void OnLostDevice(IDirect3DDevice9* /*pDevice*/, 
        D3D9HelperPtr /*pHelper*/)
      {
        m_pLine->OnLostDevice();
      }

      // OnResetDevice callback
      void OnResetDevice(IDirect3DDevice9* /*pDevice*/, 
        D3D9HelperPtr /*pHelper*/)
      {
        m_pLine->OnResetDevice();
      }

      // OnInitialize callback
      void OnInitialize(IDirect3DDevice9* pDevice, D3D9HelperPtr /*pHelper*/)
      {
        D3DXCreateLine(pDevice, &m_pLine);
      }

      // Draw line
      void DrawLine(Math::Vec2f const& Start, Math::Vec2f const& End, 
        float Width, D3DCOLOR Color)
      {
        m_pLine->SetWidth(Width);

        D3DXVECTOR2 D3DXVec[2] = 
        { 
          D3DXVECTOR2(Start[0], Start[1]), 
          D3DXVECTOR2(End[0], End[1]) 
        };

        m_pLine->Begin();
        m_pLine->Draw(D3DXVec, 2, Color);
        m_pLine->End();
      }

      // Draw box
      void DrawBox(Math::Vec2f const& BottomLeft, Math::Vec2f const& TopRight, 
        float LineWidth, D3DCOLOR Color)
      {
        // Width of box
        float Width = TopRight[0] - BottomLeft[0];
        // Height of box
        float Height = TopRight[1] - BottomLeft[1];

        // Top left corner of box
        Math::Vec2f TopLeft(TopRight[0] - Width, TopRight[1]);
        // Bottom right corner of box
        Math::Vec2f BottomRight(TopRight[0], TopRight[1] - Height);

        // Bottom left to top left
        DrawLine(BottomLeft, TopLeft, LineWidth, Color);
        // Bottom left to bottom right
        DrawLine(BottomLeft, BottomRight, LineWidth, Color);
        // Bottom right to top right
        DrawLine(BottomRight, TopRight, LineWidth, Color);
        // Top left to top right
        DrawLine(TopLeft, TopRight, LineWidth, Color);
      }

      // Get D3D line
      ID3DXLine* GetLine()
      {
        return m_pLine;
      }

    private:
      // D3D line
      ID3DXLine* m_pLine;
    };
  }
}
