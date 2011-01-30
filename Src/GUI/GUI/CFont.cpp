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

CFont::CFont( IDirect3DDevice9 * pDevice, int iHeight, const char * pszFaceName )
{
#ifdef USE_D3DX
	HRESULT hResult = D3DXCreateFontA( pDevice, -MulDiv( iHeight, GetDeviceCaps( GetDC( 0 ), LOGPIXELSY ), 72 ), 0, FW_NORMAL, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, pszFaceName, &m_pFont );

	if( FAILED( hResult ) )
		MessageBoxA( 0, DXGetErrorDescription9A( hResult ), "D3DXCreateFontA failed", 0 );

	m_pFont->PreloadCharacters( 0, 255 );
#else
	m_pFont = new CD3DFont( pszFaceName ? pszFaceName : "Arial", iHeight, 0 );
	m_pFont->Initialize( pDevice );
#endif
}

CFont::~CFont()
{
#ifdef USE_D3DX
	SAFE_RELEASE( m_pFont );
#else
	m_pFont->Invalidate();
#endif
}

void CFont::OnLostDevice()
{
#ifdef USE_D3DX
	m_pFont->OnLostDevice();
#else
	m_pFont->Invalidate();
#endif
}

void CFont::OnResetDevice( IDirect3DDevice9 * pDevice )
{
#ifdef USE_D3DX
	pDevice; // C4100: 'pDevice' : unreferenced formal parameter

	m_pFont->OnResetDevice();
#else
	m_pFont->Initialize( pDevice );
#endif
}

void CFont::DrawString( int iX, int iY, DWORD dwFlags, CColor * pColor, std::string sString, int iWidth )
{
#ifdef USE_D3DX
		gpGui->GetSprite()->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE );
		
		D3DXMATRIX mat;
		D3DXMatrixTranslation( &mat, static_cast<float>( iX ), static_cast<float>( iY ), 0 );
		gpGui->GetSprite()->SetTransform( &mat );
		
		RECT drawRect = { 0 };
		DWORD dwDrawFlags = DT_NOCLIP | ( ( dwFlags & FT_CENTER ) ? DT_CENTER : 0 ) | ( ( dwFlags & FT_VCENTER ) ? DT_VCENTER : 0 );
		m_pFont->DrawTextA( gpGui->GetSprite(), sString.c_str(), -1, &drawRect, dwDrawFlags, pColor->GetD3DColor() );
		
		gpGui->GetSprite()->End();
#else
		if( iWidth )
			CutString( iWidth, sString );
		m_pFont->Print( static_cast<float>( iX ), static_cast<float>( iY ), pColor->GetD3DColor(), sString.c_str(), dwFlags );
#endif
}

int CFont::GetStringWidth( const char * pszString ) const
{
#ifdef USE_D3DX
	std::string sString( pszString );
	RECT rRect = { 0 };

	for( int i = 0; i <= static_cast<int>( sString.size() ); i++ )
		if( sString[i] == ' ' )
			sString[i] = '.';

	m_pFont->DrawTextA( 0, sString.c_str(), -1, &rRect, DT_CALCRECT, 0 );

	return rRect.right - rRect.left;
#else
	return m_pFont->DrawLength( (char*)pszString );
#endif
}

int CFont::GetStringHeight() const
{
#ifdef USE_D3DX
	RECT rRect = { 0 };
	m_pFont->DrawTextA( 0, "Y", -1, &rRect, DT_CALCRECT, 0 );

	return rRect.bottom - rRect.top;
#else
	return m_pFont->DrawHeight();
#endif
}

void CFont::CutString( int iMaxWidth, std::string & rString ) const
{
	int iIndex = 0, iLength = rString.length();

	for( int iWidth = 0; iIndex < iLength && iWidth + 10 < iMaxWidth; )
	{
		char szCurrent[ 2 ] = { rString.c_str()[ iIndex ], 0 };
		iWidth += gpGui->GetFont()->GetStringWidth( szCurrent );
		iIndex++;
	}

	if( iIndex < iLength )
		rString[ iIndex - 1 ] = '\0';
}