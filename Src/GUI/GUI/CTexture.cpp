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
#include "CGUI.h"

CTexture::~CTexture()
{
	SAFE_RELEASE( m_pTexture );
}

CTexture::CTexture( ID3DXSprite * pSprite, const char * szPath ) : m_bAlpha( 255 )
{
	pSprite->GetDevice( &m_pDevice );

	D3DXCreateTextureFromFileA( m_pDevice, szPath, &m_pTexture );

	if( !m_pTexture )
	{
		std::stringstream sStream;
		sStream << "Failed to load texture: " << szPath;
		MessageBoxA( 0, sStream.str().c_str(), "CTexture::CTexture( ... )", 0 );
		return;
	}

	m_pTexture->GetLevelDesc( 0, &m_TexDesc );
	m_pSprite = pSprite;
}

IDirect3DTexture9 * CTexture::GetTexture() const
{
	return m_pTexture;
}
void CTexture::SetAlpha( BYTE bAlpha )
{
	m_bAlpha = bAlpha;
}

void CTexture::Draw( CPos cpPos, int iWidth, int iHeight )
{
	D3DXVECTOR2 vTransformation = D3DXVECTOR2( static_cast<float>( cpPos.GetX() ), static_cast<float>( cpPos.GetY() ) );

	D3DXVECTOR2 vScaling( ( 1.0f / m_TexDesc.Width ) * iWidth, ( 1.0f / m_TexDesc.Height ) * iHeight );

	D3DXMATRIX mainMatrix;

	D3DXMatrixTransformation2D( &mainMatrix, 0, 0, &vScaling, 0, 0, &vTransformation );

	m_pSprite->SetTransform( &mainMatrix );

	m_pSprite->Begin( D3DXSPRITE_ALPHABLEND );
	m_pSprite->Draw( m_pTexture, 0, 0, 0, D3DCOLOR_RGBA( 255, 255, 255, m_bAlpha ) );
	m_pSprite->End();
}