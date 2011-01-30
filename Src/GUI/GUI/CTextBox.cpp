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

CTextBox::CTextBox( TiXmlElement * pElement )
{
	SetElement( pElement );

	pSlider = new CHelperSlider( CPos( GetWidth() - HELPERSLIDER_WIDTH, 0 ), GetHeight() );

	for( TiXmlElement * pString = pElement->FirstChildElement( "Row" ); pString; pString = pString->NextSiblingElement( "Row" ) )
		AddString( pString->GetText() );

	SetThemeElement( gpGui->GetThemeElement( "TextBox" ) );

	if( !GetThemeElement() )
		MessageBoxA( 0, "Theme element invalid.", "TextBox", 0 );
	else
		SetElementState( "Norm" );
}

void CTextBox::Draw()
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	gpGui->DrawOutlinedBox( Pos.GetX(), Pos.GetY(), GetWidth(), GetHeight(), pInner->GetD3DColor(), pBorder->GetD3DColor() );

	int iAddHeight = gpGui->GetFont()->GetStringHeight();
	if( m_vStrings.size() )
		for( int i = pSlider->GetValue(), iHeight = 0; i <= pSlider->GetMaxValue() && iHeight < GetHeight() - gpGui->GetFont()->GetStringHeight(); i++ )
		{
			gpGui->GetFont()->DrawString( Pos.GetX() + 3, Pos.GetY() + iHeight, 0, pString, m_vStrings[ i ], GetWidth() - HELPERSLIDER_WIDTH );
			iHeight += iAddHeight;
		}

	pSlider->Draw( Pos );
}

void CTextBox::PreDraw()
{
	pSlider->PreDraw();
}

void CTextBox::MouseMove( CMouse & pMouse )
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	SetMouseOver( pMouse.InArea( Pos.GetX(), Pos.GetY(), GetWidth(), GetHeight() ) );

	pSlider->MouseMove( Pos, pMouse );
}

bool CTextBox::KeyEvent( SKey sKey )
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	if( GetMouseOver() || ( !sKey.m_bDown && !gpGui->GetMouse().GetWheel() )  )
		return pSlider->KeyEvent( Pos, sKey );

	return true;
}

void CTextBox::AddString( std::string sString )
{
	if( !sString.length() )
		return;

	std::vector<std::string> vPending;
	int iLength = static_cast<int>( sString.length() );
	for( int i = iLength - 1; i > 0; i-- )
	{
		if( sString[ i ] == '\n' )
		{
			sString[ i ] = '\0';
			
			if( i + 1 < iLength )
			{
				if( sString[ i + 1 ] == '\r' )
				{
					if( i + 2 < iLength )
						vPending.push_back( &sString.c_str()[ i + 2 ] );
				}
				else
					vPending.push_back( &sString.c_str()[ i + 1 ] );
			}
		}
	}

	pSlider->SetMaxValue( m_vStrings.size() );
	m_vStrings.push_back( sString.c_str() );

	int iHeight = 0;
	for( int i = pSlider->GetValue(); i <= pSlider->GetMaxValue(); i++ )
	{
		float fWidth = static_cast<float>( gpGui->GetFont()->GetStringWidth( m_vStrings[ i ].c_str() ) );
		int iLines = static_cast<int>( ceilf( fWidth / ( GetWidth() - HELPERSLIDER_WIDTH ) ) );

		int iTempHeight = iLines*gpGui->GetFont()->GetStringHeight();
		iHeight += iTempHeight;
	
		while( iHeight > GetHeight() - gpGui->GetFont()->GetStringHeight() )
		{
			pSlider->SetValue( pSlider->GetValue() + iLines );
			iHeight -= iTempHeight;
		}
	}

	for( std::vector<std::string>::reverse_iterator iIter = vPending.rbegin(); iIter != vPending.rend(); iIter++ )
		AddString( *iIter );
}

void CTextBox::Clear()
{
	m_vStrings.clear();
}

void CTextBox::UpdateTheme( int iIndex )
{
	SElementState * pState = GetElementState( iIndex );

	pString = pState->GetColor( "String" );
	pInner = pState->GetColor( "Inner" );
	pBorder = pState->GetColor( "Border" );
}