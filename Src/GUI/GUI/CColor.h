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

#include <map>
#include "TinyXML\TinyXML.h"

class CColor
{
	D3DCOLOR m_d3dColor;
public:
	CColor();
	CColor( int iRed, int iGreen, int iBlue, int iAlpha );
	CColor( D3DCOLOR d3dColor );
	CColor( TiXmlElement * pElement );
	~CColor();

	void SetD3DColor( D3DCOLOR d3dColor );
	void SetRed( int iRed );
	void SetGreen( int iGreen );
	void SetBlue( int iBlue );
	void SetAlpha( int iAlpha );

	D3DCOLOR GetD3DColor() const;
	int GetRed() const;
	int GetGreen() const;
	int GetBlue() const;
	int GetAlpha() const;

	const CColor operator / ( const int iDivisor ) const;
	const CColor operator * ( const int iMultiplicator ) const;

	const CColor operator - ( const CColor & cSubColor ) const;
	const CColor operator + ( const CColor & cAddColor ) const;
};

struct SElement;

struct SElementState
{
	SElement * pParent;

	CColor * GetColor( std::string sString ) const;
	CTexture * GetTexture( std::string sString ) const;

	std::map<std::string, CColor*> mColors;
	std::map<std::string, CTexture*> mTextures;
};

struct SElement
{
	std::string sDefaultState;
	std::map<std::string, SElementState*> m_mStates;
};