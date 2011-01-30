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

CProgressBar::CProgressBar( TiXmlElement * pElement )
{
	SetElement( pElement );

	int iProgress = 0;
	pElement->QueryIntAttribute( "value", &iProgress );
	SetProgress( iProgress );

	SetThemeElement( gpGui->GetThemeElement( "ProgressBar" ) );

	if( !GetThemeElement() )
		MessageBoxA( 0, "Theme element invalid.", "ProgressBar", 0 );
	else
		SetElementState( "Norm" );
}

void CProgressBar::Draw()
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	gpGui->DrawOutlinedBox( Pos.GetX(), Pos.GetY(), GetWidth(), GetHeight(), pInner->GetD3DColor(), pBorder->GetD3DColor() );

	if( GetProgress() )
		pProgressBar->Draw( CPos( Pos.GetX() + 2, Pos.GetY() + 2 ), static_cast<int>( ( static_cast<float>( GetWidth() ) - 4 ) / 100 * GetProgress() ), GetHeight() - 4 );

	gpGui->GetFont()->DrawString( Pos.GetX() + GetWidth() / 2, Pos.GetY() + GetHeight() / 2, FT_CENTER|FT_VCENTER, pString, m_sBuf );
}

int CProgressBar::GetProgress() const
{
	return m_iProgress;
}

void CProgressBar::SetProgress( int iProgress )
{
	if( iProgress > 100 )
		iProgress = 100;
	else if( iProgress < 0 )
		iProgress = 0;

	m_iProgress = iProgress;

	std::stringstream sStream;
	sStream << GetProgress() << static_cast<char>( 37 );
	m_sBuf = sStream.str();

	m_iStrWidth = gpGui->GetFont()->GetStringWidth( m_sBuf.c_str() ) / 2;
	m_iStrHeight = gpGui->GetFont()->GetStringHeight() / 2;
}

void CProgressBar::UpdateTheme( int iIndex )
{
	SElementState * pState = GetElementState( iIndex );

	pInner = pState->GetColor( "Inner" );
	pBorder = pState->GetColor( "Border" );
	pString = pState->GetColor( "String" );

	pProgressBar = pState->GetTexture( "ProgressBar" );
}