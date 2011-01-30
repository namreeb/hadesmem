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

void CElement::SetElement( TiXmlElement * pElement )
{
	SetMouseOver( false );

	if( !pElement )
		return;

	int iTempX = 0, iTempY = 0;

	if( pElement->QueryIntAttribute( "relX", &iTempX ) == TIXML_NO_ATTRIBUTE )
		iTempX = 0;
	if( pElement->QueryIntAttribute( "relY", &iTempY ) == TIXML_NO_ATTRIBUTE )
		iTempY = 0;

	SetRelPos( CPos( iTempX, iTempY ) );

	if( pElement->QueryIntAttribute( "absX", &iTempX ) == TIXML_NO_ATTRIBUTE )
		iTempX = 0;
	if( pElement->QueryIntAttribute( "absY", &iTempY ) == TIXML_NO_ATTRIBUTE )
		iTempY = 0;

	SetAbsPos( CPos( iTempX, iTempY ) );

	if( pElement->QueryIntAttribute( "width", &iTempX ) == TIXML_NO_ATTRIBUTE )
		iTempX = 0;
	if( pElement->QueryIntAttribute( "height", &iTempY ) == TIXML_NO_ATTRIBUTE )
		iTempY = 0;

	SetWidth( iTempX );
	SetHeight( iTempY );

	const char * pszString = pElement->Attribute( "string" );
	if( pszString )
		SetString( pszString );

	pszString = pElement->Attribute( "string2" );
	if( pszString )
		SetString( pszString, 1 );

	const char * pszCallback = pElement->Attribute( "callback" );
	if( pszCallback )
	{
		SetCallback( gpGui->GetCallback( pszCallback ) );

		if( !GetCallback() )
			MessageBoxA( 0, "Callback invalid", pszCallback, 0 );
	}
	else
		SetCallback( 0 );

	SetMouseOver( false );
}

void CElement::SetParent( CWindow * pParent )
{
	m_pParent = pParent;
}

CWindow * CElement::GetParent() const
{
	return m_pParent;
}

void CElement::SetCallback( tCallback pCallback )
{
	m_pCallback = pCallback;
}

tCallback CElement::GetCallback() const
{
	return m_pCallback;
}

void CElement::SetRelPos( CPos relPos )
{
	m_relPos = relPos;
}

const CPos * CElement::GetRelPos() const
{
	return &m_relPos;
}

void CElement::SetAbsPos( CPos absPos )
{
	m_absPos = absPos;
}

const CPos * CElement::GetAbsPos() const
{
	return &m_absPos;
}

void CElement::SetWidth( int iWidth )
{
	m_iWidth = iWidth;
}

int CElement::GetWidth() const
{
	return m_iWidth;
}

void CElement::SetHeight( int iHeight )
{
	m_iHeight = iHeight;
}

int CElement::GetHeight() const
{
	return m_iHeight;
}

bool CElement::HasFocus() const
{
	return GetParent()->GetFocussedElement() == this;
}

void CElement::SetString( std::string sString, int iIndex )
{
	if( static_cast<int>( sString.length() ) > 255 )
		return;

	m_sRaw[ iIndex ] = sString;
}

std::string CElement::GetString( bool bReplaceVars, int iIndex )
{
	std::string & sFormatted = m_sFormatted[ iIndex ] = m_sRaw[ iIndex ];

	if( bReplaceVars )
	{
		std::string::size_type sPos = 0;
		while( ( sPos = sFormatted.find( "$", sPos ) ) != std::string::npos )
		{
			for( std::map<std::string,tCallback>::const_reverse_iterator iIter = gpGui->GetCallbackMap().rbegin(); iIter != gpGui->GetCallbackMap().rend(); iIter++ )
			{
				const std::string & sName = iIter->first;

				if( !sFormatted.compare( sPos, sName.length(), sName ) )
				{
					sFormatted = sFormatted.replace( sPos, sName.length(), iIter->second( 0, this ) );
					break;
				}
			}
			sPos++;
		}

		return sFormatted;
	}

	return m_sRaw[ iIndex ];
}

std::string CElement::GetFormatted( int iIndex ) const
{
	return m_sFormatted[ iIndex ];
}

bool CElement::GetMouseOver() const
{
	return m_bMouseOver;
}

bool CElement::SetMouseOver( bool bMouseOver )
{
	return m_bMouseOver = bMouseOver;
}

SElement * CElement::SetThemeElement( SElement * pThemeElement, int iIndex )
{
	return m_pThemeElement[ iIndex ] = pThemeElement;
}

SElement * CElement::GetThemeElement( int iIndex ) const
{
	return m_pThemeElement[ iIndex ];
}

void CElement::SetElementState( std::string sState, int iIndex )
{
	m_pElementState[ iIndex ] = GetThemeElement( iIndex )->m_mStates[ sState ];

	if( !m_pElementState )
		m_pElementState[ iIndex ] = GetThemeElement( iIndex )->m_mStates[ GetThemeElement( iIndex )->sDefaultState ];

	UpdateTheme( iIndex );
}

SElementState * CElement::GetElementState( int iIndex ) const
{
	return m_pElementState[ iIndex ];
}

void CElement::UpdateTheme( int )
{
}

void CElement::Draw()
{
}

void CElement::PreDraw()
{
}

void CElement::MouseMove( CMouse & )
{
}

bool CElement::KeyEvent( SKey )
{
	return true;
}
