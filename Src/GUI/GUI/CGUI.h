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

// #define USE_D3DX

#define TITLEBAR_HEIGHT 24
#define BUTTON_HEIGHT	20
#define HELPERSLIDER_WIDTH 20

#define SAFE_DELETE( pData ) if( pData ){ delete pData; pData = 0; }

class CTexture;
class CTimer;
class CPos;
class CColor;
class CMouse;
class CKeyboard;
class CElement;
class CWindow;
class CButton;
class CCheckBox;
class CProgressBar;
class CTextBox;
class CListBox;

#include <windowsx.h>
#include <shlwapi.h>

#include <map>
#include <set>
#include <limits>
#include <iomanip>
#include <sstream>
#include <vector>

#include "D3D9.h"
#include "CD3DRender.h"
#include "TinyXML\tinyxml.h"

#include "CTexture.h"
#include "CFont.h"

#include "CTimer.h"

#include "CPos.h"
#include "CColor.h"

#include "CMouse.h"
#include "CKeyboard.h"

#include "CElement.h"
#include "CWindow.h"
#include "CHorizontalSliderBar.h"
#include "CVerticalSliderBar.h"
#include "CHelperSlider.h"
#include "CButton.h"
#include "CCheckBox.h"
#include "CProgressBar.h"
#include "CText.h"
#include "CEditBox.h"
#include "CDropDown.h"
#include "CTextBox.h"
#include "CListBox.h"

typedef std::string ( __cdecl * tCallback )( const char * pszArgs, CElement * pElement );

class CGUI
{
	bool m_bVisible, m_bReload;

	CMouse * m_pMouse;
	CKeyboard * m_pKeyboard;
	CFont * m_pFont;

	IDirect3DDevice9 * m_pDevice;

	ID3DXSprite * m_pSprite;

#ifdef USE_D3DX
	ID3DXLine * m_pLine;
#else
	CD3DRender * m_pRender;
#endif
	
	CTimer m_tPreDrawTimer;

	std::vector<CWindow*> m_vWindows;

	std::string m_sCurTheme;

	typedef std::map<std::string, SElement*> tTheme;
	std::map<std::string, tTheme> m_mThemes;

	std::map<std::string, tCallback> m_mCallbacks;
public:

	CGUI( IDirect3DDevice9 * pDevice );
	~CGUI();

	void LoadInterfaceFromFile( const char * pszFilePath );

	void FillArea( int iX, int iY, int iWidth, int iHeight, D3DCOLOR d3dColor );
	void DrawLine( int iStartX, int iStartY, int iEndX, int iEndY, int iWidth, D3DCOLOR d3dColor );
	void DrawOutlinedBox( int iX, int iY, int iWidth, int iHeight, D3DCOLOR d3dInnerColor, D3DCOLOR d3dBorderColor );

	CWindow * AddWindow( CWindow * pWindow );
	void BringToTop( CWindow * pWindow );

	void Draw();
	void PreDraw();
	void MouseMove( CMouse & pMouse );
	bool KeyEvent( SKey sKey );

	void OnLostDevice();
	void OnResetDevice( IDirect3DDevice9 * pDevice );

	CMouse & GetMouse() const;
	CKeyboard * GetKeyboard() const;

	IDirect3DDevice9 * GetDevice() const;
	CFont * GetFont() const;
	ID3DXSprite * GetSprite() const;

	CWindow * GetWindowByString( std::string sString, int iIndex = 0 );

	SElement * GetThemeElement( std::string sElement ) const;

	void SetVisible( bool bVisible );
	bool IsVisible() const;

	bool ShouldReload() const;
	void Reload();

	tCallback const & GetCallback( std::string sString ) const
	{
		return m_mCallbacks.find( sString )->second;
	}
	void AddCallback( std::string sString, tCallback pCallback )
	{
		m_mCallbacks[ sString ] = pCallback;
	}
	std::map<std::string,tCallback> const & GetCallbackMap() const
	{
		return m_mCallbacks;
	}
};

extern CGUI * gpGui;