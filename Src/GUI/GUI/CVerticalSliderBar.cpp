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

CVerticalSliderBar::CVerticalSliderBar( TiXmlElement * pElement )
{
	SetDragged( false );
	SetCallback( 0 );
	m_iMinValue = 0, m_iMaxValue = 0, m_iValue = 0;

	SetSliderElement( pElement );

	SetThemeElement( gpGui->GetThemeElement( "VerticalSliderBar" ) );

	if( !GetThemeElement() )
		MessageBoxA( 0, "Theme element invalid.", "VerticalSliderBar", 0 );
	else
		SetElementState( "Norm" );
}

void CVerticalSliderBar::Draw()
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	D3DCOLOR d3dLineColor = pLines->GetD3DColor();
	
	gpGui->DrawLine( Pos.GetX() + TITLEBAR_HEIGHT / 2, Pos.GetY(),					Pos.GetX() + TITLEBAR_HEIGHT / 2,		Pos.GetY() + GetHeight(), 1,		d3dLineColor );
	gpGui->DrawLine( Pos.GetX() + TITLEBAR_HEIGHT / 4, Pos.GetY(),					Pos.GetX() + TITLEBAR_HEIGHT / 4 * 3,	Pos.GetY(), 1,						d3dLineColor );
	gpGui->DrawLine( Pos.GetX() + TITLEBAR_HEIGHT / 4, Pos.GetY() + GetHeight(),		Pos.GetX() + TITLEBAR_HEIGHT / 4 * 3,	Pos.GetY() + GetHeight(), 1,		d3dLineColor );
	gpGui->DrawLine( Pos.GetX() + TITLEBAR_HEIGHT / 4, Pos.GetY() + GetHeight() / 2,	Pos.GetX() + TITLEBAR_HEIGHT / 4 * 3,	Pos.GetY() + GetHeight() / 2, 1,	d3dLineColor );

	pSlider->Draw( CPos( Pos.GetX() + 2, Pos.GetY() + GetHeight() - static_cast<int>( floor( static_cast<float>( GetHeight() ) / GetMaxValue() * GetValue() ) ) - 5 ), BUTTON_HEIGHT, 10 );

	gpGui->GetFont()->DrawString( Pos.GetX() - TITLEBAR_HEIGHT + ( TITLEBAR_HEIGHT * 3 ) / 2, Pos.GetY() + GetHeight() + 5, FT_CENTER, pString, GetFormatted() );
}

void CVerticalSliderBar::MouseMove( CMouse & pMouse )
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	if( GetDragged() )
	{
		CPos mousePos = pMouse.GetPos();

		if( mousePos.GetX() == -1 && mousePos.GetY() == -1 )
			mousePos = pMouse.GetSavedPos();

			if( mousePos.GetY() < Pos.GetY() )
				SetValue( GetMaxValue() );
			else if( mousePos.GetY() > Pos.GetY() + GetHeight() )
				SetValue( GetMinValue() );
			else
			{
				for( int iIndex = GetMinValue(); iIndex < GetMaxValue(); iIndex++ )
					if( mousePos.GetY() >= Pos.GetY() + floor( static_cast<float>( GetHeight() ) / GetMaxValue() * iIndex ) && mousePos.GetY() <= Pos.GetY() + floor( (float)GetHeight() / GetMaxValue() * ( iIndex + 1 ) ) )
					{
						SetValue( GetMaxValue() - iIndex );
						break;
					}
			}

			if( GetCallback() )
				GetCallback()( reinterpret_cast<const char*>( GetValue() ), this );
	}
	else
		SetElementState( SetMouseOver( gpGui->GetMouse().InArea( Pos.GetX(), Pos.GetY(), TITLEBAR_HEIGHT, GetHeight() ) )?"MouseOver":"Norm" );
}

void CVerticalSliderBar::UpdateTheme( int iIndex )
{
	SElementState * pState = GetElementState( iIndex );

	pLines = pState->GetColor( "Lines" );
	pString = pState->GetColor( "String" );

	pSlider = pState->GetTexture( "Slider" );
}