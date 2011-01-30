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

CHorizontalSliderBar::CHorizontalSliderBar()
{
	SetDragged( false );
	SetCallback( 0 );
	m_iMinValue = 0, m_iMaxValue = 0, m_iValue = 0;
}

CHorizontalSliderBar::CHorizontalSliderBar( TiXmlElement * pElement )
{
	SetDragged( false );
	SetCallback( 0 );
	m_iMinValue = 0, m_iMaxValue = 0, m_iValue = 0;

	SetSliderElement( pElement );

	SetThemeElement( gpGui->GetThemeElement( "HorizontalSliderBar" ) );

	if( !GetThemeElement() )
		MessageBoxA( 0, "Theme element invalid.", "HorizontalSliderBar", 0 );
	else
		SetElementState( "Norm" );
}

void CHorizontalSliderBar::SetSliderElement( TiXmlElement * pElement )
{
	SetElement( pElement );

	if( pElement->Attribute( "string" ) )
		SetShowString( false );
	else
		SetShowString( true );

	int iTempValue = 0;

	if( pElement->QueryIntAttribute( "minvalue", &iTempValue ) != TIXML_NO_ATTRIBUTE )
		SetMinValue( iTempValue );
	else
		SetMinValue( 0 );

	if( pElement->QueryIntAttribute( "maxvalue", &iTempValue ) != TIXML_NO_ATTRIBUTE )
		SetMaxValue( iTempValue );
	else
		SetMaxValue( 0 );

	if( pElement->QueryIntAttribute( "step", &iTempValue ) != TIXML_NO_ATTRIBUTE )
		SetStep( iTempValue );
	else
		SetMaxValue( 0 );

	if( pElement->QueryIntAttribute( "value", &iTempValue ) != TIXML_NO_ATTRIBUTE )
		SetValue( iTempValue );
	else
		SetMaxValue( 0 );

	TiXmlElement * pUpdater = pElement->FirstChildElement( "Updater" );
	if( pUpdater )
	{
		const char * pszCallback = pUpdater->GetText();
		if( pszCallback )
		{
			m_pUpdater = gpGui->GetCallback( pszCallback );
			
			if( !m_pUpdater )
				MessageBoxA( 0, "Callback invalid", pszCallback, 0 );
		}
	}
	else
		m_pUpdater = 0;
}

void CHorizontalSliderBar::Draw()
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	SElementState * pState = GetElementState();
	if( pState )
	{
		D3DCOLOR d3dLineColor = pLines->GetD3DColor();

		gpGui->DrawLine( Pos.GetX(),					Pos.GetY() + TITLEBAR_HEIGHT / 2,Pos.GetX() + GetWidth(),		Pos.GetY() + TITLEBAR_HEIGHT / 2,		1, d3dLineColor );
		gpGui->DrawLine( Pos.GetX(),					Pos.GetY() + TITLEBAR_HEIGHT / 4, Pos.GetX(),					Pos.GetY() + TITLEBAR_HEIGHT / 4 * 3,	1, d3dLineColor );
		gpGui->DrawLine( Pos.GetX() + GetWidth(),		Pos.GetY() + TITLEBAR_HEIGHT / 4, Pos.GetX() + GetWidth(),		Pos.GetY() + TITLEBAR_HEIGHT / 4 * 3,	1, d3dLineColor );
		gpGui->DrawLine( Pos.GetX() + GetWidth() / 2,	Pos.GetY() + TITLEBAR_HEIGHT / 4, Pos.GetX() + GetWidth() / 2,	Pos.GetY() + TITLEBAR_HEIGHT / 4 * 3,	1, d3dLineColor );

		pSlider->Draw( CPos( Pos.GetX() + static_cast<int>( floor( static_cast<float>( GetWidth() ) / ( GetMaxValue() - GetMinValue() ) * ( GetValue() - GetMinValue() ) ) ) - 5, Pos.GetY() + 2 ), 10, TITLEBAR_HEIGHT - 4 );

		gpGui->GetFont()->DrawString( Pos.GetX() + GetWidth() / 2, Pos.GetY() - 15, FT_CENTER, pString, GetFormatted() );
	}
}

void CHorizontalSliderBar::PreDraw()
{
	if( !GetFormatted().length() )
		GetString( !GetShowString() );

	if( m_pUpdater )
		m_pUpdater( reinterpret_cast<const char*>( GetValue() ), this );
}

void CHorizontalSliderBar::MouseMove( CMouse & pMouse )
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	if( GetDragged() )
	{
		CPos mousePos = pMouse.GetPos();

		if( mousePos.GetX() == -1 && mousePos.GetY() == -1 )
			mousePos = pMouse.GetSavedPos();

			if( mousePos.GetX() < Pos.GetX() )
				SetValue( GetMinValue() );
			else if( mousePos.GetX() > Pos.GetX() + GetWidth() )
				SetValue( GetMaxValue() );
			else
			{
				for( int iIndex = GetMinValue(); iIndex < GetMaxValue(); iIndex++ )
					if( mousePos.GetX() >= Pos.GetX() + floor( static_cast<float>( GetWidth() ) / ( GetMaxValue() - GetMinValue() ) * ( iIndex - GetMinValue() ) ) && mousePos.GetX() <= Pos.GetX() + floor( static_cast<float>( GetWidth() ) / ( GetMaxValue() - GetMinValue() ) * ( iIndex + 1 - GetMinValue() ) ) )
					{
						SetValue( iIndex );
						break;
					}
			}

			if( GetCallback() )
				GetCallback()( reinterpret_cast<const char*>( GetValue() ), this );
	}
	else
		SetElementState( SetMouseOver( gpGui->GetMouse().InArea( Pos.GetX(), Pos.GetY(), GetWidth(), TITLEBAR_HEIGHT ) )?"MouseOver":"Norm" );
}

bool CHorizontalSliderBar::KeyEvent( SKey sKey )
{
	if( !sKey.m_vKey )
	{
		SetDragged( GetMouseOver() && gpGui->GetMouse().GetLeftButton() );

		SetElementState( GetDragged()?"Pressed":( GetMouseOver()?"MouseOver":"Norm" ) );

		if( GetDragged() )
			MouseMove( gpGui->GetMouse() );
	}

	return true;
}

int CHorizontalSliderBar::GetMinValue() const
{
	return m_iMinValue;
}

int CHorizontalSliderBar::GetMaxValue() const
{
	return m_iMaxValue;
}

int CHorizontalSliderBar::GetValue() const
{
	return m_iValue;
}

int CHorizontalSliderBar::GetStep() const
{
	return m_iStep;
}

void CHorizontalSliderBar::SetMinValue( int iMinValue )
{
	m_iMinValue = iMinValue;
}

void CHorizontalSliderBar::SetMaxValue( int iMaxValue )
{
	m_iMaxValue = iMaxValue;
}

void CHorizontalSliderBar::SetValue( int iValue )
{
	if( iValue > GetMaxValue() )
		m_iValue = GetMaxValue();
	else if( iValue < GetMinValue() )
		m_iValue = GetMinValue();
	else
		m_iValue = iValue;

	if( GetShowString() )
	{
		std::stringstream sStream;
		sStream << GetValue() << "/" << GetMaxValue();
		SetString( sStream.str() );
		GetString();
	}
	else
		GetString( true );
}

void CHorizontalSliderBar::SetStep( int iStep )
{
	m_iStep = iStep;
}

bool CHorizontalSliderBar::GetDragged() const
{
	return m_bDragged;
}

void CHorizontalSliderBar::SetDragged( bool bDragged )
{
	m_bDragged = bDragged;

	if( m_bDragged )
		SetElementState( "Pressed" );
}

void CHorizontalSliderBar::SetShowString( bool bShow )
{
	m_bShowString = bShow;
}

bool CHorizontalSliderBar::GetShowString() const
{
	return m_bShowString;
}

void CHorizontalSliderBar::UpdateTheme( int iIndex )
{
	SElementState * pState = GetElementState( iIndex );

	pLines = pState->GetColor( "Lines" );
	pString = pState->GetColor( "String" );

	pSlider = pState->GetTexture( "Slider" );
}

// SliderBar specific GUI cfunctions
std::string MinValue( const char *, CElement * pElement )
{
	std::stringstream sStream;

	if( pElement )
	{
		CHorizontalSliderBar * pSlider = (CHorizontalSliderBar*)pElement;

		try
		{
			sStream << pSlider->GetMinValue();
		}
		catch( ... )
		{
			MessageBoxA( 0, "$MinValue failed", pElement->GetString().c_str(), 0 );
		}
	}
	return sStream.str();
}

std::string MaxValue( const char *, CElement * pElement )
{
	std::stringstream sStream;

	if( pElement )
	{
		CHorizontalSliderBar * pSlider = (CHorizontalSliderBar*)pElement;

		try
		{
			sStream << pSlider->GetMaxValue();
		}
		catch( ... )
		{
			MessageBoxA( 0, "$MaxValue failed", pElement->GetString().c_str(), 0 );
		}
	}
	return sStream.str();
}

std::string SliderValue( const char *, CElement * pElement )
{
	std::stringstream sStream;

	if( pElement )
	{
		CHorizontalSliderBar * pSlider = (CHorizontalSliderBar*)pElement;

		try
		{
			sStream << pSlider->GetValue();
		}
		catch( ... )
		{
			MessageBoxA( 0, "$Value failed", pElement->GetString().c_str(), 0 );
		}
	}
	return sStream.str();
}
// SliderBar specific GUI cfunctions