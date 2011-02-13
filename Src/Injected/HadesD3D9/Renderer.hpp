/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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

// Windows API
#include <Windows.h>
#include <atlbase.h>

// DirectX
#include <d3d9.h>
#include <d3dx9.h>

// Hades
#include "HadesRenderer/Renderer.hpp"

namespace Hades
{
  namespace GUI
  {
    // Todo: Make things like font height, font face, text colour, draw flags, 
    // etc configurable.
    
    class D3D9Renderer : public Renderer
    {
    public:
      explicit D3D9Renderer(IDirect3DDevice9* pDevice) 
        : m_pDevice(pDevice), 
        m_pStateBlock(), 
        m_pFont()
      {
        HRESULT ResultSB = m_pDevice->CreateStateBlock(D3DSBT_ALL, 
          &m_pStateBlock);
  
        if (FAILED(ResultSB))
        {
          std::error_code const MyErrorCode(ResultSB, std::system_category());
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("D3D9Renderer::D3D9Renderer") << 
            ErrorString("Could not create font.") << 
            ErrorCode(MyErrorCode));
        }
        
        UINT Height = 10;
        HRESULT ResultF = D3DXCreateFont(
          m_pDevice, 
          -MulDiv(Height, GetDeviceCaps(GetDC(0), LOGPIXELSY), 72), 
          0, 
          FW_NORMAL, 
          0, 
          0, 
          DEFAULT_CHARSET, 
          OUT_DEFAULT_PRECIS, 
          DEFAULT_QUALITY, 
          DEFAULT_PITCH | FF_DONTCARE, 
          L"Arial Bold", 
          &m_pFont);
  
        if (FAILED(ResultF))
        {
          std::error_code const MyErrorCode(ResultF, std::system_category());
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("D3D9Renderer::D3D9Renderer") << 
            ErrorString("Could not create font.") << 
            ErrorCode(MyErrorCode));
        }
      }
      
      void PreReset()
      {
        m_pStateBlock = nullptr;
      }
      
      void PostReset()
      {
        m_pDevice->CreateStateBlock(D3DSBT_ALL, &m_pStateBlock);
      }
      
      void DrawText(std::wstring const& Text, unsigned int X, unsigned int Y)
      {
        m_pStateBlock->Capture();
        
        RECT DrawRect = { X, Y };
        m_pFont->DrawText(
          nullptr, 
          Text.c_str(), 
          -1, 
          &DrawRect, 
          DT_NOCLIP, 
          D3DCOLOR_RGBA(0, 255, 0, 255));
        
        m_pStateBlock->Apply();
      }
      
    private:
      IDirect3DDevice9* m_pDevice;
      CComPtr<IDirect3DStateBlock9> m_pStateBlock;
      CComPtr<ID3DXFont> m_pFont;
    };
  }
}
