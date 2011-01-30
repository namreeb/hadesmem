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

#include "CGUI.h"

class CMouse
{
	IDirect3DDevice9 * m_pDevice;
	CPos m_pos, m_bpos;

	CColor * m_pInnerColor, * m_pBorderColor;

	CElement * m_pDraggingElement;

	int m_iLeftButton, m_iRightButton, m_iMiddleButton, m_iWheel;
	CTimer m_tLeftButton, m_tRightButton, m_tMiddleButton;
public:
	CMouse( IDirect3DDevice9 * pDevice );
	~CMouse();

	bool HandleMessage( unsigned int uMsg, WPARAM wParam, LPARAM lParam );

	void SetPos( int iX, int iY );
	void SetPos( CPos cPos );
	CPos GetPos() const;

	bool InArea( int iX, int iY, int iWidth, int iHeight ) const;
	bool InArea( CElement * pElement, int iHeight = 0 ) const;

	void Draw();

	int GetLeftButton( int iState = -1 );
	int GetRightButton( int iState = -1 );
	int GetMiddleButton( int iState = -1 );
	int GetWheel( int iState = -1 );

	void SetLeftButton( int iState );
	void SetRightButton( int iState );
	void SetMiddleButton( int iState );
	void SetWheel( int iState );

	void SetDragging( CElement * pElement );
	CElement * GetDragging() const;

	void SavePos();
	void LoadPos();
	CPos GetSavedPos() const;
};