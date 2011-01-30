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

CWindow::CWindow( TiXmlElement * pElement )
{
	SetMaximized( true );
	SetFocussedElement( 0 );
	posDif = CPos();
	m_bDragging = false;
	SetMouseOver( false );
	SetElement( pElement );

	const char * pszVisible = pElement->Attribute( "hidden" );
	if( pszVisible )
		SetVisible( pszVisible[ 0 ] != '1' );
	else
		SetVisible( true );

	#define LOAD_ELEMENTS( Class, String ) \
	for( TiXmlElement * pElementElement = pElement->FirstChildElement( String ); pElementElement; pElementElement = pElementElement->NextSiblingElement( String ) ) \
		AddElement( new Class( pElementElement ) );

	LOAD_ELEMENTS( CButton, "Button" )
	LOAD_ELEMENTS( CText, "Text" )
	LOAD_ELEMENTS( CProgressBar, "ProgressBar" )
	LOAD_ELEMENTS( CCheckBox, "CheckBox" )
	LOAD_ELEMENTS( CEditBox, "EditBox" )
	LOAD_ELEMENTS( CHorizontalSliderBar, "HorizontalSliderBar" )
	LOAD_ELEMENTS( CVerticalSliderBar, "VerticalSliderBar" )
	LOAD_ELEMENTS( CTextBox, "TextBox" )
	LOAD_ELEMENTS( CListBox, "ListBox" )
	LOAD_ELEMENTS( CDropDown, "DropDown" )

	SetThemeElement( gpGui->GetThemeElement( "Window" ) );

	if( !GetThemeElement() )
		MessageBoxA( 0, "Theme element invalid.", "Window", 0 );
	else
		SetElementState( "Norm" );

	SetThemeElement( gpGui->GetThemeElement( "CloseButton" ), 1 );

	if( !GetThemeElement( 1 ) )
		MessageBoxA( 0, "Theme element invalid.", "CloseButton", 0 );
	else
	{
		SetElementState( "Norm", 1 );

		pszVisible = pElement->Attribute( "closebutton" );
		if( pszVisible )
			SetCloseButton( pszVisible[ 0 ] == '1' );
		else
			SetCloseButton( true );

		MouseMove( gpGui->GetMouse() );
	}
}

CWindow::~CWindow()
{
	for each( CElement * pElement in m_vElements )
		SAFE_DELETE( pElement )
	m_vElements.clear();
}

void CWindow::AddElement( CElement * pElement )
{
	pElement->SetRelPos( *pElement->GetRelPos() + CPos( 0, TITLEBAR_HEIGHT ) );
	pElement->SetParent( this );

	m_vElements.push_back( pElement );
}

void CWindow::Draw()
{	
	pTitlebar->Draw( *GetAbsPos(), GetWidth(), TITLEBAR_HEIGHT );
	gpGui->GetFont()->DrawString( GetAbsPos()->GetX() + 5, GetAbsPos()->GetY() + 5, 0, pTitle, GetFormatted() );
	pButton->Draw( CPos( GetAbsPos()->GetX() + GetWidth() - BUTTON_HEIGHT - 2, GetAbsPos()->GetY() + 2 ), BUTTON_HEIGHT, BUTTON_HEIGHT );

	if( GetMaximized() )
	{
		gpGui->DrawOutlinedBox( GetAbsPos()->GetX(), GetAbsPos()->GetY() + TITLEBAR_HEIGHT, GetWidth(), GetHeight() - TITLEBAR_HEIGHT + 1,  pBodyInner->GetD3DColor(), pBodyBorder->GetD3DColor() );

		for each( CElement * pElement in m_vElements )
			pElement->Draw();
	}
}

void CWindow::PreDraw()
{
	GetString( true );

	if( GetMaximized() )
		for each( CElement * pElement in m_vElements )
			pElement->PreDraw();
}

void CWindow::MouseMove( CMouse & pMouse )
{
	if( GetDragging() )
	{
		if( !posDif.GetX() )
			posDif = *GetAbsPos() - pMouse.GetPos();
		else
		{
			CPos mPos = pMouse.GetPos();

			if( mPos.GetX() == -1 && mPos.GetY() == -1 )
				mPos = pMouse.GetSavedPos();

			SetAbsPos( mPos + posDif );
		}
	}

	if( GetCloseButton() )
		SetElementState( SetMouseOver( pMouse.InArea( GetAbsPos()->GetX() + GetWidth() - BUTTON_HEIGHT - 2, GetAbsPos()->GetY() + 2, BUTTON_HEIGHT, BUTTON_HEIGHT ) )?"MouseOver":"Norm", 1 );

	if( GetMaximized() )
		for each( CElement * pElement in m_vElements )
			pElement->MouseMove( pMouse );
}

bool CWindow::KeyEvent( SKey sKey )
{
	CMouse & Mouse = gpGui->GetMouse();

	if( Mouse.GetLeftButton() )
	{
		SetFocussedElement( 0 );

		if( GetMouseOver() && m_bCloseButtonEnabled )
			this->SetVisible( false );
		else if( Mouse.InArea( GetAbsPos()->GetX(), GetAbsPos()->GetY(), GetWidth(), TITLEBAR_HEIGHT ) )
		{
			if( !Mouse.GetDragging() )
			{
				if( Mouse.GetLeftButton() == 1 )
				{
					gpGui->BringToTop( this );

					SetDragging( true );
					Mouse.SetDragging( this );

					SetElementState( "Dragging" );
				}
				else
				{
					SetMaximized( !GetMaximized() );

					SetElementState( GetMaximized()?"Norm":"Minimized" );

					gpGui->BringToTop( this );
				}
			}
		}
		else if(  Mouse.InArea( GetAbsPos()->GetX(), GetAbsPos()->GetY(), GetWidth(), GetHeight() ) )
			gpGui->BringToTop( this );
	}
	else
	{
		posDif.SetX( 0 );

		Mouse.SetDragging( 0 );
		SetDragging( false );

		SetElementState( GetMaximized()?"Norm":"Minimized" );
	}

	if( GetMaximized() )
		for( int iIndex = static_cast<int>( m_vElements.size() ) - 1; iIndex >= 0; iIndex-- )
			if( !m_vElements[ iIndex ]->KeyEvent( sKey ) )
				return false;
	return true;
}

void CWindow::SetMaximized( bool bMaximized )
{
	m_bMaximized = bMaximized;
}

bool CWindow::GetMaximized()
{
	return m_bMaximized;
}

void CWindow::SetVisible( bool bVisible )
{
	m_bVisible = bVisible;
}

bool CWindow::IsVisible()
{
	return m_bVisible;
}

void CWindow::SetDragging( bool bDragging )
{
	m_bDragging = bDragging;
}

bool CWindow::GetDragging()
{
	return m_bDragging;
}

void CWindow::SetCloseButton( bool bEnabled )
{
	m_bCloseButtonEnabled = bEnabled;

	if( GetCloseButton() )
		SetElementState( "Disabled", 1 );
}

bool CWindow::GetCloseButton()
{
	return m_bCloseButtonEnabled;
}

void CWindow::SetFocussedElement( CElement * pElement )
{
	m_pFocussedElement = pElement;

	if( pElement )
		BringToTop( pElement );
}

CElement * CWindow::GetFocussedElement()
{
	return m_pFocussedElement;
}

CElement * CWindow::GetElementByString( const char * pszString, int iIndex )
{
	for each( CElement * pElement in m_vElements )
		if( pElement->GetString( false, iIndex ) == pszString )
			return pElement;
	return 0;
}

void CWindow::BringToTop( CElement * pElement )
{
	for( int i = 0; i < static_cast<int>( m_vElements.size() ); i++ )
		if( m_vElements[i] == pElement )
			m_vElements.erase( m_vElements.begin() + i );
	m_vElements.insert(  m_vElements.end(), pElement );
}

void CWindow::UpdateTheme( int iIndex )
{
	SElementState * pState = GetElementState( iIndex );
	if( !iIndex )
	{
		pTitle = pState->GetColor( "Title" );
		pBodyInner = pState->GetColor( "BodyInner" );
		pBodyBorder = pState->GetColor( "BodyBorder" );

		pTitlebar = pState->GetTexture( "Titlebar" );
	}
	else
		pButton = pState->GetTexture( "Button" );
}
