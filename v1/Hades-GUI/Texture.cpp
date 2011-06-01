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
#include "Error.h"
#include "Texture.h"

namespace Hades
{
  namespace GUI
  {
    Texture::Texture(ID3DXSprite* pSprite, std::string const& Path) 
      : m_pTexture(), 
      m_TexDesc(), 
      m_pSprite(pSprite), 
      m_pDevice(nullptr), 
      m_Alpha(255)
    {
      pSprite->GetDevice(&m_pDevice);

      HRESULT Result = D3DXCreateTextureFromFileA(m_pDevice, Path.c_str(), 
        &m_pTexture);
      if (FAILED(Result))
      {
        BOOST_THROW_EXCEPTION(HadesGuiError() << 
          ErrorFunction("Texture::Texture") << 
          ErrorString("Could not create texture.") << 
          ErrorCodeWin(Result));
      }

      m_pTexture->GetLevelDesc(0, &m_TexDesc);
    }

    IDirect3DTexture9* Texture::GetTexture() const
    {
      return m_pTexture;
    }

    void Texture::SetAlpha(BYTE Alpha)
    {
      m_Alpha = Alpha;
    }

    void Texture::Draw(Pos MyPos, int Width, int Height)
    {
      D3DXMATRIX MainMat;

      D3DXVECTOR2 VecScaling((1.0f / m_TexDesc.Width) * Width, 
        (1.0f / m_TexDesc.Height) * Height);

      D3DXVECTOR2 VecTransform = D3DXVECTOR2(static_cast<float>(MyPos.GetX()), 
        static_cast<float>(MyPos.GetY()));

      D3DXMatrixTransformation2D(&MainMat, 0, 0, &VecScaling, 0, 0, &VecTransform);

      m_pSprite->SetTransform(&MainMat);

      m_pSprite->Begin(D3DXSPRITE_ALPHABLEND);

      m_pSprite->Draw(m_pTexture, 0, 0, 0, D3DCOLOR_RGBA(255, 255, 255, 
        m_Alpha));

      m_pSprite->End();
    }

    void Texture::SetSprite(ID3DXSprite* pSprite)
    {
      m_pSprite = pSprite;
    }
  }
}
