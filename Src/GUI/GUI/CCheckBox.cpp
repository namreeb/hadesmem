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

CCheckBox::CCheckBox( TiXmlElement * pElement )
{
	SetElement( pElement );

	int iChecked = 0;
	pElement->QueryIntAttribute( "checked", &iChecked );

	SetChecked( iChecked == 1 );

	SetThemeElement( gpGui->GetThemeElement( "CheckBox" ) );

	if( !GetThemeElement() )
		MessageBoxA( 0, "No color scheme element found.", "CheckBox", 0 );
	else
		SetElementState( "Norm" );
}

bool CCheckBox::GetChecked() const
{
	return m_bChecked;
}
void CCheckBox::SetChecked( bool bChecked )
{
	m_bChecked = bChecked;
}

void CCheckBox::Draw()
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	gpGui->DrawOutlinedBox( Pos.GetX(), Pos.GetY(), 15, 15, pInner->GetD3DColor(), pBorder->GetD3DColor() );
	gpGui->GetFont()->DrawString( Pos.GetX() + 20, Pos.GetY(), 0, pString, GetFormatted() );

	if( GetChecked() )
	{
		D3DCOLOR d3dCrossColor = pCross->GetD3DColor();

		gpGui->DrawLine( Pos.GetX() + 1, Pos.GetY() + 1, Pos.GetX() + 14, Pos.GetY() + 14, 1, d3dCrossColor );
		gpGui->DrawLine( Pos.GetX() + 1, Pos.GetY() + 13, Pos.GetX() + 14, Pos.GetY(), 1, d3dCrossColor );
	}
}

void CCheckBox::PreDraw()
{
	GetString( true );
}

void CCheckBox::MouseMove( CMouse & pMouse )
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	SetElementState( SetMouseOver( pMouse.InArea( Pos.GetX(), Pos.GetY(), gpGui->GetFont()->GetStringWidth( GetFormatted().c_str() ) + 20, gpGui->GetFont()->GetStringHeight() ) )?"MouseOver":"Norm" );
}

bool CCheckBox::KeyEvent( SKey sKey )
{
	if( !sKey.m_vKey )
	{
		if( GetMouseOver() && gpGui->GetMouse().GetLeftButton( 0 ) )
		{
			m_bChecked = !m_bChecked;
			
			if( GetCallback() )
				GetCallback()( (const char*)m_bChecked, this );
		}
	}

	return true;
}

void CCheckBox::UpdateTheme( int iIndex )
{
	SElementState * pState = GetElementState( iIndex );

	pInner = pState->GetColor( "Inner" );
	pBorder = pState->GetColor( "Border" );
	pString = pState->GetColor( "String" );
	pCross = pState->GetColor( "Cross" );
}