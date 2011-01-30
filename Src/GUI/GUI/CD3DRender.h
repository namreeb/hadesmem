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
#ifndef _D3DRENDER_H
#define _D3DRENDER_H

#include <d3d9.h>

class CD3DFont;
class CD3DRender;

#define FCR_NONE	0x0
#define FCR_BOLD 	0x1
#define FCR_ITALICS 0x2

#define FT_NONE		0x0
#define FT_CENTER	0x1
#define FT_BORDER	0x2
#define FT_VCENTER	0x4
#define FT_SINGLELINE 0x8

//FVF Macros
#define D3DFVF_BITMAPFONT	(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#define D3DFVF_PRIMITIVES	(D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

//Vertex Types
struct d3dfont_s { float x,y,z,rhw; DWORD colour; float tu,tv; };
typedef struct { float x,y,z,rhw; DWORD colour; } d3dprimitives_s;

//Initialization Macros
#undef SAFE_RELEASE
#define SAFE_RELEASE( pInterface ) if( pInterface ) { pInterface->Release(); pInterface = 0; }

class CD3DBaseRender
{
public:
	CD3DBaseRender();
	~CD3DBaseRender();

	static HRESULT BeginRender();
	static HRESULT EndRender();

protected:
	static HRESULT Initialize(IDirect3DDevice9 *pD3Ddev);
	static HRESULT Invalidate();

	static HRESULT CreateStates();
	static HRESULT DeleteStates();

	static IDirect3DDevice9		*m_pD3Ddev;
	static IDirect3DStateBlock9	*m_pD3DstateDraw;
	static IDirect3DStateBlock9	*m_pD3DstateNorm;

	static int	m_renderCount;
	static int	m_numShared;
	static bool m_statesOK;
};

class CD3DRender : public CD3DBaseRender
{
public:
	CD3DRender(int numVertices);
	~CD3DRender();

	HRESULT Initialize(IDirect3DDevice9 *pD3Ddev);
	HRESULT Invalidate();

	HRESULT Begin(D3DPRIMITIVETYPE primType);
	HRESULT End();

	inline HRESULT D3DColour(DWORD colour);
	inline HRESULT D3DVertex2f(float x, float y);

	HRESULT D3DAddQuad( int x, int y, int w, int h, DWORD dwColor );
	
private:
	D3DPRIMITIVETYPE		m_primType;
	IDirect3DVertexBuffer9	*m_pD3Dbuf;
	d3dprimitives_s			*m_pVertex;

	DWORD					m_colour;
	int						m_maxVertex;
	int						m_curVertex;

	bool					m_canRender;
};

class CD3DFont : public CD3DBaseRender
{
public:
	CD3DFont( const char *szFontName, int fontHeight, DWORD dwCreateFlags = 0 );
	~CD3DFont();

	HRESULT Initialize(IDirect3DDevice9 *pD3Ddev);
	HRESULT Invalidate();

	HRESULT Print( float x, float y, DWORD colour, const char *szText, DWORD dwFlags = 0 );

	int DrawLength( const char* ) const;
	inline int DrawHeight() const
	{
		return static_cast<int>( m_fChrHeight );
	}

private:	
	HRESULT CreateVertexBuffers();
	HRESULT DeleteVertexBuffers();

	char 	m_szFontName[31+1];
	int  	m_fontHeight;
	DWORD 	m_dwCreateFlags;

	bool 	m_isReady;

	IDirect3DTexture9 		*m_pD3Dtex;
	IDirect3DVertexBuffer9 	*m_pD3Dbuf;
	CD3DRender				*m_pRender;

	DWORD 	m_maxTriangles;

	int 	m_texWidth, m_texHeight;
	int 	m_chrSpacing;
	float 	m_fTexCoords[256][4];
	float 	m_fChrHeight;		
};

#endif