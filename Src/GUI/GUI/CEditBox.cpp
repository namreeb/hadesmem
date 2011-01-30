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

CEditBox::CEditBox( TiXmlElement * pElement )
{
	SetElement( pElement );
	SetHeight( BUTTON_HEIGHT );

	m_iStart = 0;
	SetIndex( 0 );
	m_bCursorState = false;
	SetCallback( 0 );

	const char * pszCallback = pElement->Attribute( "callback" );
	if( pszCallback )
	{
		SetCallback( gpGui->GetCallback( pszCallback ) );

		if( !GetCallback() )
			MessageBoxA( 0, "Callback invalid", pszCallback, 0 );
	}

	SetThemeElement( gpGui->GetThemeElement( "EditBox" ) );

	if( !GetThemeElement() )
		MessageBoxA( 0, "Theme element invalid.", "EditBox", 0 );
	else
		SetElementState( "Norm" );
}

void CEditBox::Draw()
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	SElementState * pState = GetElementState();
	if( pState )
	{
		gpGui->DrawOutlinedBox( Pos.GetX(), Pos.GetY(), GetWidth(), GetHeight(), pInner->GetD3DColor(), pBorder->GetD3DColor() );
		
		std::string sTemp = &GetString()[ GetStart() ];
		gpGui->GetFont()->CutString( GetWidth(), sTemp );

		gpGui->GetFont()->DrawString( Pos.GetX() + 4, Pos.GetY() + GetHeight() / 2, FT_VCENTER, pString, sTemp );
		
		if( m_bCursorState )
			gpGui->FillArea( Pos.GetX() + 2 + m_iCursorX, Pos.GetY() + 2, 2, GetHeight() - 4, pCursor->GetD3DColor() );
	}
}

void CEditBox::PreDraw()
{
	if( !m_tCursorTimer.Running() && ( HasFocus() || m_bCursorState ) )
	{
		m_bCursorState = !m_bCursorState;
		m_tCursorTimer.Start( 0.6f );
	}
}

void CEditBox::MouseMove( CMouse & pMouse )
{
	CPos Pos = *GetParent()->GetAbsPos() + *GetRelPos();

	SetMouseOver( pMouse.InArea( Pos.GetX(), Pos.GetY(), GetWidth(), GetHeight() ) );
}

bool CEditBox::KeyEvent( SKey sKey )
{
	if( !sKey.m_vKey )
	{
		if( gpGui->GetMouse().GetLeftButton() )
		{
			if( GetMouseOver() )
			{
				int iX = gpGui->GetMouse().GetPos().GetX();
				int iAbsX = ( *GetParent()->GetAbsPos() + *GetRelPos() ).GetX();

				std::string sString( &GetString()[ GetStart() ] );
				
				if( iX >= iAbsX + gpGui->GetFont()->GetStringWidth( sString.c_str() ) )
					SetIndex( sString.length() );
				else
				{
					for( int i = 0; i <= static_cast<int>( sString.length() ); i++ )
					{
						if( iX <= iAbsX + gpGui->GetFont()->GetStringWidth( sString.c_str() ) )
						{
							sString[ i ] = 0;
							if( iX > iAbsX + gpGui->GetFont()->GetStringWidth( sString.c_str() ) )
								SetIndex( i );
						}
						sString = &GetString()[ GetStart() ];
					}
				}

				GetParent()->SetFocussedElement( this );
			}
		}
	}
	else if( sKey.m_bDown && HasFocus() )
	{
		switch( sKey.m_vKey )
		{
		case VK_END:
			{
				std::string sString = GetString();

				SetIndex( strlen( &sString[ GetStart() ] ) );

				while( gpGui->GetFont()->GetStringWidth( &sString.c_str()[ GetStart() ] ) > GetWidth() - 5 || m_iCursorX > GetWidth() - 5 )
				{
					SetStart( GetStart() + 1 );
					SetIndex( GetIndex() - 1 );
				}
				break;
			}
		case VK_HOME:
			{
				SetStart( 0 );
				SetIndex( 0 );

				break;
			}
		case VK_BACK:
			{
				if( GetIndex() )
				{
					std::string sString = GetString();

					sString.erase( GetStart() + GetIndex() - 1,  1 );

					SetString( sString );
					SetIndex( GetIndex() - 1 );
				}
				else if( GetStart() )
				{
					SetStart( GetStart() - 1 );
					SetIndex( 1 );
				}

				break;
			}
		case VK_DELETE:
			{
				std::string sString = GetString();

				if( GetIndex() <= static_cast<int>( sString.length() ) )
					sString.erase( GetStart() + m_iIndex, 1 );

				SetString( const_cast<char*>( sString.c_str() ) );

				break;
			}
		case VK_LEFT:
			{
				if( !GetIndex() && GetStart() )
					SetStart( GetStart() - 1 );
				else if( GetIndex() )
					SetIndex( GetIndex() - 1 );

				break;
			}
		case VK_RIGHT:
			{
				SetIndex( GetIndex() + 1 );

				std::string sString = GetString();
				sString[ GetIndex() ] = 0;

				while( gpGui->GetFont()->GetStringWidth( &sString.c_str()[ GetStart() ] ) > GetWidth() - 5 || m_iCursorX > GetWidth() - 5 )
				{
					SetStart( GetStart() + 1 );
					SetIndex( GetIndex() - 1 );
				}

				break;
			}
		case VK_RETURN:
			{
				GetParent()->SetFocussedElement( 0 );

				tCallback pAction = GetCallback();

				if( pAction )
					pAction( GetString().c_str(), this );

				break;
			}
		default:
			{
				std::string sString( GetString() );

				int iPrevLen = sString.length();

				BYTE bKeys[256] = { 0 };
				GetKeyboardState( bKeys );

				WORD wKey = 0;
				ToAscii( sKey.m_vKey, HIWORD( sKey.m_lParam )&0xFF, bKeys, &wKey, 0 );

				char szKey[2] = { static_cast<char>( wKey ), 0 };
				if( GetStart() + m_iIndex >= 0 && GetStart() + m_iIndex <= static_cast<int>( sString.length() ) )
				{
					if( wKey != 22 )
						sString.insert( GetStart() + m_iIndex, szKey );
					else
					{
						if( !OpenClipboard( 0 ) )
							break;

						HANDLE hData = GetClipboardData( CF_TEXT );
						char * pszBuffer = static_cast<char*>( GlobalLock( hData ) );
						
						if( pszBuffer )
							sString.insert( GetStart() + m_iIndex, pszBuffer );

						GlobalUnlock( hData );
						CloseClipboard();
					}
				}

				SetString( const_cast<char*>( sString.c_str() ) );
				if( sKey.m_vKey == ' ' )
					SetIndex( GetIndex() + 1 );
				else
					SetIndex( GetIndex() + sString.length() - iPrevLen );

				while( gpGui->GetFont()->GetStringWidth( &GetString().c_str()[ GetStart() ] ) > GetWidth() - 5 )
				{
					SetStart( GetStart() + 1 );
					SetIndex( GetIndex() - 1 );
				}

				break;
			}
		}
	}

	return true;
}

int CEditBox::GetIndex() const
{
	return m_iIndex;
}

void CEditBox::SetIndex( int iIndex )
{
	std::string sString( &GetString()[ GetStart() ] );

	if( iIndex > static_cast<int>( sString.length() ) || iIndex < 0 )
		return;

	sString[ iIndex ] = 0;

	m_iCursorX = gpGui->GetFont()->GetStringWidth( sString.c_str() );

	m_iIndex = iIndex;
}

int CEditBox::GetStart() const
{
	return m_iStart;
}

void CEditBox::SetStart( int iStart )
{
	m_iStart = iStart;
}

void CEditBox::UpdateTheme( int iIndex )
{
	SElementState * pState = GetElementState( iIndex );

	pInner = pState->GetColor( "Inner" );
	pBorder = pState->GetColor( "Border" );
	pString = pState->GetColor( "String" );
	pCursor = pState->GetColor( "Cursor" );
}
