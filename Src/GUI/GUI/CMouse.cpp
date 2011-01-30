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

#define DBLCLICK_TIME 0.3f

CMouse::CMouse( IDirect3DDevice9 * pDevice )
{
	m_pDevice = pDevice;

	m_pInnerColor = new CColor( 255, 255, 255, 255 );
	m_pBorderColor = new CColor( 0, 0, 0, 255 );

	SetLeftButton( 0 );
	SetRightButton( 0 );
	SetMiddleButton( 0 );
	SetWheel( 0 );

	SetDragging( 0 );
}

CMouse::~CMouse()
{
}

bool CMouse::HandleMessage( unsigned int uMsg, WPARAM wParam, LPARAM lParam )
{
	if( !gpGui->IsVisible() || uMsg < WM_MOUSEFIRST || uMsg > WM_MOUSELAST )
		return false;

	bool bDown = false;

	switch( uMsg )
	{
	case WM_MOUSEMOVE:
		{
			SetPos( GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) );
			gpGui->MouseMove( *this );
			return false;
		}

	case WM_LBUTTONDOWN:
		SetLeftButton( 1 );
		bDown = true;
		break;
	case WM_RBUTTONDOWN:
		SetRightButton( 1 );
		bDown = true;
		break;
	case WM_MBUTTONDOWN:
		SetMiddleButton( 1 );
		bDown = true;
		break;

	case WM_LBUTTONUP:
		SetLeftButton( 0 );
		break;
	case WM_RBUTTONUP:
		SetRightButton( 0 );
		break;
	case WM_MBUTTONUP:
		SetMiddleButton( 0 );
		break;

	case WM_MOUSEWHEEL:
		float fDelta = GET_WHEEL_DELTA_WPARAM( wParam );
		
		if( fDelta > 0.0f )
			SetWheel( 1 );
		else if( fDelta < 0.0f )
			SetWheel( 2 );
		else
			SetWheel( 0 );

		break;
	}

	return gpGui->KeyEvent( SKey( 0, bDown ) );
}

void CMouse::SetPos( CPos cPos )
{
	m_pos = cPos;
}

void CMouse::SetPos( int iX, int iY )
{
	m_pos.SetX( iX );
	m_pos.SetY( iY );
}

CPos CMouse::GetPos() const
{
	return m_pos;
}

bool CMouse::InArea( int iX, int iY, int iWidth, int iHeight ) const
{
	return ( m_pos.GetX() >= iX && m_pos.GetX() <= iX + iWidth && m_pos.GetY() >= iY && m_pos.GetY() <= iY + iHeight );
}

bool CMouse::InArea( CElement * pElement, int iHeight ) const
{
	if( !iHeight )
		iHeight = pElement->GetHeight();

	return InArea( pElement->GetAbsPos()->GetX(), pElement->GetAbsPos()->GetY(), pElement->GetWidth(), iHeight );
}

void CMouse::Draw()
{
}

int CMouse::GetLeftButton( int iState )
{
	int iRet = m_iLeftButton;

	if( iState != -1 )
		SetLeftButton( iState );

	return iRet;
}

int CMouse::GetRightButton( int iState )
{
	int iRet = m_iRightButton;

	if( iState != -1 )
		SetRightButton( iState );

	return iRet;
}

int CMouse::GetMiddleButton( int iState )
{
	int iRet = m_iMiddleButton;

	if( iState != -1 )
		SetMiddleButton( iState );

	return iRet;
}

int CMouse::GetWheel( int iState )
{
	int iRet = m_iWheel;

	if( iState != -1 )
		SetWheel( iState );

	return iRet;
}

void CMouse::SetLeftButton( int iState )
{
	if( iState == 1 )
	{
		if( m_tLeftButton.Running() )
		{
			m_iLeftButton = 2;
			m_tLeftButton.Stop();
		}
		else
		{
			m_iLeftButton = 1;
			m_tLeftButton.Start( DBLCLICK_TIME );
		}
	}
	else
		m_iLeftButton = iState;
}

void CMouse::SetRightButton( int iState )
{
	if( iState == 1 )
	{
		if( m_tRightButton.Running() )
		{
			m_iRightButton = 2;
			m_tRightButton.Stop();
		}
		else
		{
			m_iRightButton = 1;
			m_tRightButton.Start( DBLCLICK_TIME );
		}
	}
	else
		m_iRightButton = iState;
}

void CMouse::SetMiddleButton( int iState )
{
	if( iState == 1 )
	{
		if( m_tMiddleButton.Running() )
		{
			m_iMiddleButton = 2;
			m_tMiddleButton.Stop();
		}
		else
		{
			m_iMiddleButton = 1;
			m_tMiddleButton.Start( DBLCLICK_TIME );
		}
	}
	else
		m_iMiddleButton = iState;
}

void CMouse::SetWheel( int iState )
{
	m_iWheel = iState;
}

void CMouse::SetDragging( CElement * pElement )
{
	m_pDraggingElement = pElement;
}

CElement * CMouse::GetDragging() const
{
	return m_pDraggingElement;
}

void CMouse::SavePos()
{
	m_bpos = m_pos;
}

void CMouse::LoadPos()
{
	m_pos = m_bpos;
}

CPos CMouse::GetSavedPos() const
{
	return m_bpos;
}