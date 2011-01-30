////////////////////////////////////////////////////////////////////////
// This font class was created by Azorbix (Matthew L) for Game-Deception
// http://www.game-deception.com   http://forum.game-deception.com
// irc://irc.rizon.net/game-deception
//
// A lot all of the CD3DFont::Initialize() code was created by Microsoft
// (taken from the D3D9 SDK)
// 
// Please note that this is NOT 100% complete yet, colour tags and 
// shadows are not implemented yet
//
// USAGE:
//   CD3DFont:
//     1) Instanciate the class with the parameterized constructor
//        eg CD3DFont *g_pD3Dfont = new CD3DFont("Arial", 16, FCT_BOLD);
//
//     2) Call Initialize() after other rendering is ready
//        eg g_pD3DFont->Initialize(pD3Ddevice);
//
//     3) To begin rendering use Print function
//        eg g_pD3DFont->Print(10.0f, 50.0f, 0xFF00FF00, "Hello World", FT_BORDER);
//
//     4) call Invalidate() upon Reset of the D3D surface and re-initialize
//
//   CD3DRender:
//     1) Instanciate the class with the parameterized constructor
//        eg CD3DRender *g_pRender = new CD3DRender(128);
//
//     2) Call Initialize() after other rendering is ready
//        eg g_pRender->Initialize(pD3Ddevice);
//
//     3) To begin rendering, start rendering much like OpenGL
//        eg if( SUCCEEDED(g_pRender->Begin(D3DPT_TRIANGLELIST)) )
//           {
//               D3DAddQuad(g_pRender, 10.0f, 10.0f, 50.0f, 50.0f, 0xFFFF0000); //helper function
//               g_pRender->D3DColour(0xFF0000FF); //blue
//               g_pRender->D3DVertex2f(60.0f, 60.0f);
//               g_pRender->D3DVertex2f(60.0f, 110.0f);
//               g_pRender->D3DVertex2f(110.0f, 110.0f);
//               g_pRender->End();
//           }
//
//     4) call Invalidate() upon Reset of the D3D surface and re-initialize
//
// FASTER RENDERING (Advanced but NOT REQUIRED):
//   To enable faster rendering, it's ideal to call static function CD3DBaseRendering::BeginRender(); before
//   other font / primitive rendering code, and call CD3DBaseRendering::EndRender(); afterwards
//   *** IT IS CRUCIAL THAT YOU CALL EndRender FOR EVERY BeginRender() CALL ***
//   *** IMAGE RENDERING MAY BECOME CORRUPT IF YOU DO NOT ***
//   eg
//     if( SUCCEEDED(CD3DBaseRender::BeginRender()) )
//     {
//         //primitive and font rendering goes here
//         CD3DBaseRender::EndRender();
//     }
//

#include <d3d9.h>
#include <cmath>
#include <stdio.h>
#include <wchar.h>
#include <string>
#include "CD3DRender.h"

#undef _DEBUG
#define _DEBUG(s)

IDirect3DDevice9 * CD3DBaseRender::m_pD3Ddev = NULL;
IDirect3DStateBlock9 * CD3DBaseRender::m_pD3DstateDraw = NULL;
IDirect3DStateBlock9 * CD3DBaseRender::m_pD3DstateNorm = NULL;
int	CD3DBaseRender::m_renderCount = 0;
int	CD3DBaseRender::m_numShared = 0;
bool CD3DBaseRender::m_statesOK = false;

inline d3dprimitives_s InitPrim2DVertex(float x, float y, DWORD colour)
{
	d3dprimitives_s v = { x, y, 1.0f, 1.0f, colour };
	return v;
}

inline d3dfont_s InitFont2DVertex(float x, float y, DWORD colour, float tu, float tv)
{
	d3dfont_s v = { x, y, 1.0f, 1.0f, colour, tu, tv };
	return v;
}

CD3DBaseRender::CD3DBaseRender()
{
	m_numShared++;
}

CD3DBaseRender::~CD3DBaseRender()
{
	if( !( --m_numShared ) )
		DeleteStates();
}

HRESULT CD3DBaseRender::Initialize( IDirect3DDevice9 * pD3Ddev )
{
	if( m_pD3Ddev == NULL && ( m_pD3Ddev = pD3Ddev ) == NULL )
	{
		_DEBUG("m_pD3Ddev is NULL");
		return E_FAIL;
	}
	
	if( !m_statesOK && FAILED( CreateStates() ) )
	{
		_DEBUG( "CreateStates() failed" );
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CD3DBaseRender::Invalidate()
{
	DeleteStates();
	return S_OK;		
}

HRESULT CD3DBaseRender::BeginRender()
{
	if( !m_statesOK )
	{
		_DEBUG("::BeginRender() m_statesOK = false");
		return E_FAIL;
	}

	m_renderCount++;
	if( m_renderCount == 1 )
	{
		m_pD3DstateNorm->Capture();
		m_pD3DstateDraw->Apply();
	}

	return S_OK;
}

HRESULT CD3DBaseRender::EndRender()
{
	if( !m_statesOK )
	{
		_DEBUG("::EndRender() m_statesOK = false");
		return E_FAIL;
	}

	m_renderCount--;

	if( m_renderCount == 0)
		m_pD3DstateNorm->Apply();
	else if(m_renderCount < 0)
		m_renderCount = 0;

	return S_OK;
}

HRESULT CD3DBaseRender::CreateStates()
{
    for( int iStateBlock = 0; iStateBlock < 2; iStateBlock++ )
	{
		m_pD3Ddev->BeginStateBlock();
		m_pD3Ddev->SetPixelShader( 0 );
		m_pD3Ddev->SetVertexShader( 0 );

        m_pD3Ddev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
        m_pD3Ddev->SetRenderState( D3DRS_SRCBLEND,   D3DBLEND_SRCALPHA );
        m_pD3Ddev->SetRenderState( D3DRS_DESTBLEND,  D3DBLEND_INVSRCALPHA );
        m_pD3Ddev->SetRenderState( D3DRS_ALPHATESTENABLE,  TRUE );
        m_pD3Ddev->SetRenderState( D3DRS_ALPHAREF,         0x08 );
        m_pD3Ddev->SetRenderState( D3DRS_ALPHAFUNC,  D3DCMP_GREATEREQUAL );
        m_pD3Ddev->SetRenderState( D3DRS_FILLMODE,   D3DFILL_SOLID );
        m_pD3Ddev->SetRenderState( D3DRS_CULLMODE,   D3DCULL_CCW );
        m_pD3Ddev->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
        m_pD3Ddev->SetRenderState( D3DRS_CLIPPING,         TRUE );
        m_pD3Ddev->SetRenderState( D3DRS_CLIPPLANEENABLE,  FALSE );
        m_pD3Ddev->SetRenderState( D3DRS_VERTEXBLEND,      D3DVBF_DISABLE );
        m_pD3Ddev->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE );
        m_pD3Ddev->SetRenderState( D3DRS_FOGENABLE,        FALSE );
        m_pD3Ddev->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA);
        m_pD3Ddev->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
        m_pD3Ddev->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
        m_pD3Ddev->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
        m_pD3Ddev->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
        m_pD3Ddev->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
        m_pD3Ddev->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
        m_pD3Ddev->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
        m_pD3Ddev->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
        m_pD3Ddev->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
        m_pD3Ddev->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
        m_pD3Ddev->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
        m_pD3Ddev->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
        m_pD3Ddev->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );

		if( iStateBlock )
			m_pD3Ddev->EndStateBlock( &m_pD3DstateDraw );
		else
			m_pD3Ddev->EndStateBlock( &m_pD3DstateNorm );
	}

	m_statesOK = true;

	return S_OK;
}

HRESULT CD3DBaseRender::DeleteStates()
{
	SAFE_RELEASE( m_pD3DstateDraw );
	SAFE_RELEASE( m_pD3DstateNorm );
	m_statesOK = false;

	return S_OK;
}

CD3DRender::CD3DRender( int numVertices )
{
	m_canRender = false;

	m_pD3Dbuf = NULL;
	m_pVertex = NULL;

	m_maxVertex = numVertices;
	m_curVertex = 0;
}


CD3DRender::~CD3DRender()
{
	Invalidate();
}

HRESULT CD3DRender::Initialize( IDirect3DDevice9 *pD3Ddev )
{
	if( !m_canRender )
	{
		if( FAILED( CD3DBaseRender::Initialize( pD3Ddev ) ) )
			return E_FAIL;
		
		if( FAILED( m_pD3Ddev->CreateVertexBuffer( m_maxVertex * sizeof( d3dprimitives_s ), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &m_pD3Dbuf, NULL ) ) )
			return E_FAIL;
	
		m_canRender = true;
	}

	return S_OK;
}


HRESULT CD3DRender::Invalidate()
{
	CD3DBaseRender::Invalidate();
	SAFE_RELEASE( m_pD3Dbuf );
    m_canRender = false;
	m_pVertex = NULL;

	return S_OK;
}


HRESULT CD3DRender::Begin( D3DPRIMITIVETYPE primType )
{
	if( !m_canRender )
		return E_FAIL;

	if( m_pVertex != NULL )
		return E_FAIL;

	if( FAILED( m_pD3Dbuf->Lock( 0, 0, reinterpret_cast<void**>( &m_pVertex ), D3DLOCK_DISCARD ) ) )
		return E_FAIL;
	
	m_primType = primType;

	return S_OK;
}


HRESULT CD3DRender::End()
{
	m_pVertex = NULL;

	if( !m_canRender )
	{
		m_curVertex = 0;
		return E_FAIL;
	}

	if( FAILED( CD3DBaseRender::BeginRender() ) )
		return E_FAIL;

	int numPrims;

	switch( m_primType )
	{
		case D3DPT_POINTLIST:
			numPrims = m_curVertex;
			break;
		case D3DPT_LINELIST:
			numPrims = static_cast<int>( floor( m_curVertex / 2.0f ) );
			break;
		case D3DPT_LINESTRIP:
			numPrims = m_curVertex - 1;
			break;
		case D3DPT_TRIANGLELIST:
			numPrims = static_cast<int>( floor( m_curVertex / 3.0f ) );
			break;
		case D3DPT_TRIANGLESTRIP:
		case D3DPT_TRIANGLEFAN:
			numPrims = m_curVertex - 2;
			break;
		default:
			numPrims = 0;
			break;
	}

	m_curVertex = 0;

	if( numPrims > 0 )
	{
		m_pD3Dbuf->Unlock();

		DWORD fvf;
		m_pD3Ddev->GetFVF( &fvf );
		m_pD3Ddev->SetFVF( D3DFVF_PRIMITIVES );

		m_pD3Ddev->SetTexture( 0, NULL );
		m_pD3Ddev->SetStreamSource( 0, m_pD3Dbuf, 0, sizeof( d3dprimitives_s ) );

		m_pD3Ddev->DrawPrimitive( m_primType, 0, numPrims );

		m_pD3Ddev->SetFVF( fvf );
	}

	CD3DBaseRender::EndRender();

	return S_OK;
}


inline HRESULT CD3DRender::D3DColour( DWORD colour )
{
	m_colour = colour;
	return ( m_canRender ? S_OK : E_FAIL );
}


inline HRESULT CD3DRender::D3DVertex2f( float x, float y )
{
	if( m_canRender && m_pVertex && ++m_curVertex < m_maxVertex )
		*m_pVertex++ = InitPrim2DVertex( x, y, m_colour );
	else
		return E_FAIL;

	return S_OK;
}

HRESULT CD3DRender::D3DAddQuad( int x, int y, int w, int h, DWORD dwColor )
{
	Begin( D3DPT_TRIANGLEFAN );
	D3DColour( dwColor );
	D3DVertex2f( static_cast<float>( x ), static_cast<float>( y ) );
	D3DVertex2f( static_cast<float>( x + w ), static_cast<float>( y ) );
	D3DVertex2f( static_cast<float>( x + w ), static_cast<float>( y + h ) );
	D3DVertex2f( static_cast<float>( x ), static_cast<float>( y + h ) );
	End();

	return S_OK;
}



CD3DFont::CD3DFont( const char * pszFontName, int iFontHeight, DWORD dwCreateFlags )
{
	strcpy_s( m_szFontName, 32, pszFontName );

	m_fontHeight = iFontHeight;
	m_dwCreateFlags = dwCreateFlags;

	m_isReady = m_statesOK = false;

	m_pD3Dtex = NULL;
	m_pD3Dbuf = NULL;

	m_maxTriangles = 255 * 4;
}

CD3DFont::~CD3DFont()
{
	Invalidate();
}


HRESULT CD3DFont::Initialize( IDirect3DDevice9 * pD3Ddev )
{
	if( FAILED( CD3DBaseRender::Initialize( pD3Ddev ) ) )
	{
		_DEBUG("::Initialize(pD3Ddev) failed");
		return E_FAIL;
	}

	m_pRender = new CD3DRender( 256 );
	if( m_pRender == NULL )
	{
		_DEBUG( "m_pRender = NULL" );
		return E_FAIL;
	}

	if( FAILED( m_pRender->Initialize( pD3Ddev ) ) )
	{
		_DEBUG( "m_pRender->Initialize failed" );
		return E_FAIL;
	}

	m_texWidth = m_texHeight = 1024;

	if( FAILED( m_pD3Ddev->CreateTexture( m_texWidth, m_texHeight, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, &m_pD3Dtex, NULL ) ) )
	{
		_DEBUG( "CreateTexture failed" );
		return E_FAIL;
	}
	
	if( FAILED( m_pD3Ddev->CreateVertexBuffer( m_maxTriangles * 3 * sizeof( d3dfont_s ), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &m_pD3Dbuf, NULL ) ) )
	{
		_DEBUG( "CreateVertexBuffer failed" );
		SAFE_RELEASE( m_pD3Dtex );
		return E_FAIL;
	}

	DWORD * pBitmapBits = 0;
	BITMAPINFO bmi;

	ZeroMemory(&bmi.bmiHeader,  sizeof( BITMAPINFOHEADER ) );
	bmi.bmiHeader.biSize        = sizeof( BITMAPINFOHEADER );
	bmi.bmiHeader.biWidth       = m_texWidth;
	bmi.bmiHeader.biHeight      = -m_texHeight;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount    = 32;

	HDC hDC = CreateCompatibleDC( 0 );
	HBITMAP hbmBitmap = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS, (void**)&pBitmapBits, NULL, 0 );
	SetMapMode( hDC, MM_TEXT );

	std::string sFaceName = m_szFontName;

	HFONT hFont = CreateFontW( -MulDiv( m_fontHeight, GetDeviceCaps( hDC, LOGPIXELSY ), 72 ), 0, 0, 0, (m_dwCreateFlags&FCR_BOLD ? FW_BOLD : FW_NORMAL), m_dwCreateFlags&FCR_ITALICS, false, false, 
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 
		VARIABLE_PITCH, std::wstring( sFaceName.begin(), sFaceName.end() ).c_str() );

	if( hFont == NULL )
	{
		_DEBUG( "hFont = NULL" );
		return E_FAIL;
	}

	SelectObject( hDC, hbmBitmap );
	SelectObject( hDC, hFont );

	SetTextColor( hDC, 0x00FFFFFF );
	SetBkColor( hDC, 0 );
	SetTextAlign( hDC, TA_TOP );

	wchar_t szStr[2] = { L' ', 0 };
	SIZE size = { 0 };

	GetTextExtentPoint32W( hDC, szStr, 1, &size);

	int x = m_chrSpacing = static_cast<int>( ceil( size.cx / 4.0f ) ), y = 0;
	m_fChrHeight = static_cast<float>( size.cy - 1 );
	
	for( int c = 32; c < 255; c++ )
	{
		szStr[ 0 ] = btowc( c );

		if( !GetTextExtentPoint32W( hDC, szStr, 1, &size ) )
			MessageBoxW( 0, szStr, L"GetTextExtentPoint32W", 0 );

		if( x + size.cx + m_chrSpacing > m_texWidth )
		{
			x = m_chrSpacing;
			y += size.cy + 1;
		}

		RECT rect = { x, y, x + size.cx, y + size.cy };
		if( !ExtTextOutW( hDC, x, y, ETO_CLIPPED, &rect, szStr, 1, 0 ) )
			MessageBoxW( 0, szStr, L"ExtTextOutW", 0 );

		//tu src + dst
		m_fTexCoords[ c - 32 ][ 0 ] = static_cast<float>( ( x + 0		- m_chrSpacing ) ) / static_cast<float>( m_texWidth );
		m_fTexCoords[ c - 32 ][ 2 ] = static_cast<float>( (x + size.cx + m_chrSpacing ) ) / static_cast<float>( m_texWidth );
        
		//tv src + dst
		m_fTexCoords[ c - 32 ][ 1 ] = static_cast<float>( ( y + 0		+ 0 ) ) / static_cast<float>( m_texHeight );
		m_fTexCoords[ c - 32 ][ 3 ] = static_cast<float>( ( y + size.cy + 0 ) ) / static_cast<float>( m_texHeight );
        
		x += size.cx + ( 2 * m_chrSpacing );
	}
	
	D3DLOCKED_RECT d3dlr = { 0 };
	m_pD3Dtex->LockRect( 0, &d3dlr, 0, 0 );
    
	BYTE * pDstRow = reinterpret_cast<BYTE*>( d3dlr.pBits );
	WORD * pDst16 = 0;
	BYTE bAlpha = 0;

	for( y = 0; y < m_texHeight; y++ )
	{
		pDst16 = reinterpret_cast<WORD*>( pDstRow );
		
		for( x = 0; x < m_texWidth; x++ )
		{
			bAlpha = static_cast<BYTE>( ( pBitmapBits[ m_texWidth * y + x ] >> 8 ) ) & 0xFF;
			*pDst16 = static_cast<WORD>( ( bAlpha << 8 ) ) | 0x0FFF;
			pDst16++;
		}
		
		pDstRow += d3dlr.Pitch;
    }
	m_pD3Dtex->UnlockRect( 0 );

	DeleteObject( hbmBitmap );
	DeleteDC( hDC );
	DeleteObject( hFont );

	m_isReady = true;

	return S_OK;
}

HRESULT CD3DFont::Invalidate()
{
	CD3DBaseRender::Invalidate();
	m_isReady = false;

	SAFE_RELEASE( m_pD3Dtex );
	SAFE_RELEASE( m_pD3Dbuf );

	m_pRender->Invalidate();

	return S_OK;
}

HRESULT CD3DFont::Print( float x, float y, DWORD colour, const char * szText, DWORD dwFlags )
{
	if( !m_isReady )
	{
		_DEBUG( "::Print() m_isReady = false" );
		return E_FAIL;
	}

    int strWidth = DrawLength( szText );

	if( dwFlags & FT_CENTER )
		x -= strWidth / 2.0f;

	if( dwFlags & FT_VCENTER )
		y -= DrawHeight() / 2;

    x -= static_cast<float>( m_chrSpacing );

	if( FAILED( CD3DBaseRender::BeginRender() ) )
	{
		_DEBUG( "::Print() BeginRender() failed" );
		return E_FAIL;
	}

	DWORD dwFVF = 0;
	m_pD3Ddev->GetFVF( &dwFVF );
	m_pD3Ddev->SetFVF( D3DFVF_BITMAPFONT );
	m_pD3Ddev->SetTexture( 0, m_pD3Dtex );
	m_pD3Ddev->SetStreamSource( 0, m_pD3Dbuf, 0, sizeof( d3dfont_s ) );

	if( *szText != '\0' )
	{
		UINT usedTriangles = 0;
		d3dfont_s *pVertex;

		if( FAILED( m_pD3Dbuf->Lock( 0, 0, reinterpret_cast<void**>( &pVertex ), D3DLOCK_DISCARD ) ) )
		{
			_DEBUG( "::Print() Lock() failed" );
			return E_FAIL;
		}

		int iLen = static_cast<int>( strlen( szText ) );
		float fX = x;
		for( int i = 0; i < iLen; i++ )
		{
			if( szText[ i ] == '\n' )
			{
				y += DrawHeight();
				x = fX;
				continue;
			}
			int c = ( szText[i]&0xFF ) - 32;

			float tx1 = m_fTexCoords[ c ][ 0 ];
			float tx2 = m_fTexCoords[ c ][ 2 ];
		    float ty1 = m_fTexCoords[ c ][ 1 ];
			float ty2 = m_fTexCoords[ c ][ 3 ];

			float w = ( tx2 - tx1 ) * m_texWidth;
			float h = ( ty2 - ty1 ) * m_texHeight;

			*pVertex++ = InitFont2DVertex( x - 0.5f, y - 0.5f, colour, tx1, ty1); //topleft
			*pVertex++ = InitFont2DVertex( x + w-0.5f, y - 0.5f, colour, tx2, ty1); //topright
			*pVertex++ = InitFont2DVertex( x - 0.5f, y + h - 0.5f, colour, tx1, ty2); //bottomleft
	
			*pVertex++ = InitFont2DVertex( x + w - 0.5f, y - 0.5f, colour, tx2, ty1 ); //topright
			*pVertex++ = InitFont2DVertex( x + w - 0.5f, y + h - 0.5f, colour, tx2, ty2 ); //bottomright
			*pVertex++ = InitFont2DVertex( x  - 0.5f, y + h - 0.5f, colour, tx1, ty2 ); //bottomleft
	
			x += w - ( m_chrSpacing * 2 );

			usedTriangles += 2;
			if( usedTriangles >= m_maxTriangles )
			{
				_DEBUG( "Please increase the max triangles" );
				break;
			}
		}

		if( usedTriangles > 0 )
		{
			m_pD3Dbuf->Unlock();
			m_pD3Ddev->DrawPrimitive( D3DPT_TRIANGLELIST, 0, usedTriangles );
		}
	}

	m_pD3Ddev->SetFVF( dwFVF );
	CD3DBaseRender::EndRender();

	return S_OK;
}

int CD3DFont::DrawLength( const char * szText ) const
{
	float fLen = 0.0f;

	for( int i = strlen( szText ) - 1; i >= 0; i-- )
	{
		int iChar = ( szText[i]&0xFF ) - 32;
		if( iChar >= 0 && iChar < 255)
			fLen += ( ( m_fTexCoords[ iChar ][ 2 ] - m_fTexCoords[ iChar ][ 0 ] ) * m_texWidth ) - m_chrSpacing * 2;
	}

	return static_cast<int>( ceilf( fLen ) );
}