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

CColor::CColor() : m_d3dColor( 0 )
{
}

CColor::CColor( int iRed, int iGreen, int iBlue, int iAlpha )
{
	SetD3DColor( D3DCOLOR_RGBA( iRed, iGreen, iBlue, iAlpha ) );
}

CColor::CColor( D3DCOLOR d3dColor )
{
	SetD3DColor( d3dColor );
}

CColor::CColor( TiXmlElement * pElement )
{
	int iColors[ 4 ] = { 0 };

	pElement->QueryIntAttribute( "r", &iColors[ 0 ] );
	pElement->QueryIntAttribute( "g", &iColors[ 1 ] );
	pElement->QueryIntAttribute( "b", &iColors[ 2 ] );
	pElement->QueryIntAttribute( "a", &iColors[ 3 ] );

	SetD3DColor( D3DCOLOR_RGBA( iColors[ 0 ], iColors[ 1 ], iColors[ 2 ], iColors[ 3 ] ) );
}

CColor::~CColor()
{
}

const CColor CColor::operator / ( const int iDivisor ) const
{
	return CColor( GetRed() / iDivisor, GetGreen() / iDivisor, GetBlue() / iDivisor, GetAlpha() );
}

const CColor CColor::operator - ( const CColor & cSubColor ) const
{
	return CColor( GetRed() - cSubColor.GetRed(), GetGreen() - cSubColor.GetGreen(), GetBlue() - cSubColor.GetBlue(), GetAlpha() );
}

const CColor CColor::operator + ( const CColor & cAddColor ) const
{
	return CColor( GetRed() + cAddColor.GetRed(), GetGreen() + cAddColor.GetGreen(), GetBlue() + cAddColor.GetBlue(), GetAlpha() );
}

const CColor CColor::operator * ( const int iMultiplicator ) const
{
	return CColor( GetRed() * iMultiplicator, GetGreen() * iMultiplicator, GetBlue() * iMultiplicator, GetAlpha() );
}

void CColor::SetD3DColor( D3DCOLOR d3dColor )
{
	m_d3dColor = d3dColor;
	//SetAlpha( d3dColor >> 24 );
	//SetRed( ( d3dColor >> 16 )&0xFF );
	//SetGreen( ( d3dColor >> 8 )&0xFF );
	//SetBlue( d3dColor&0xFF ); 
}

void CColor::SetRed( int iRed )
{
	SetD3DColor( D3DCOLOR_RGBA( iRed, GetGreen(), GetBlue(), GetAlpha() ) );
	//m_iRed = iRed;
}

void CColor::SetGreen( int iGreen )
{
	SetD3DColor( D3DCOLOR_RGBA( GetRed(), iGreen, GetBlue(), GetAlpha() ) );
	//m_iGreen = iGreen;
}

void CColor::SetBlue( int iBlue )
{
	SetD3DColor( D3DCOLOR_RGBA( GetRed(), GetGreen(), iBlue, GetAlpha() ) );
	//m_iBlue = iBlue;
}

void CColor::SetAlpha( int iAlpha )
{
	SetD3DColor( D3DCOLOR_RGBA( GetRed(), GetGreen(), GetBlue(), iAlpha ) );
	//m_iAlpha = iAlpha;
}

D3DCOLOR CColor::GetD3DColor() const
{
	return m_d3dColor;
}

int CColor::GetRed() const
{
	return ( GetD3DColor() >> 16 )&0xFF;
}

int CColor::GetGreen() const
{
	return ( GetD3DColor() >> 8 )&0xFF;
}

int CColor::GetBlue() const
{
	return GetD3DColor()&0xFF;
}

int CColor::GetAlpha() const
{
	return GetD3DColor() >> 24;
}

CColor * SElementState::GetColor( std::string sString ) const
{
	std::map<std::string, CColor*>::const_iterator iIter = mColors.find( sString );

	if( iIter == mColors.end() )
	{
		iIter = pParent->m_mStates.find( pParent->sDefaultState )->second->mColors.find( sString );

		if( iIter == pParent->m_mStates.find( pParent->sDefaultState )->second->mColors.end() )
			MessageBoxA( 0, "Color not found.", sString.c_str(), 0 );
	}

	return iIter->second;
}

CTexture * SElementState::GetTexture( std::string sString ) const
{
	std::map<std::string, CTexture*>::const_iterator iIter = mTextures.find( sString );

	if( iIter == mTextures.end() )
	{
		iIter = pParent->m_mStates.find( pParent->sDefaultState )->second->mTextures.find( sString );

		if( iIter == pParent->m_mStates.find( pParent->sDefaultState )->second->mTextures.end() )
			MessageBoxA( 0, "Texture not found.", sString.c_str(), 0 );
	}

	return iIter->second;
}
