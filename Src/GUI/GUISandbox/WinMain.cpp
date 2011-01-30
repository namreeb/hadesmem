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
#include "..\GUI\CGUI.h"

#include <wininet.h>
#include <shlobj.h>

#include <sstream>

bool g_bRunning = true;

void RenderScreen();

IDirect3DDevice9 * g_pDevice = 0;

D3DPRESENT_PARAMETERS d3dPP;

int g_iMenuSize = 0;
//CTexture * pBackground = 0;

std::string MainMenuCallback( const char *, CElement * pElement )
{
	::CWindow * pWindow = gpGui->GetWindowByString( pElement->GetString( false, 1 ), 1 );

	if( pWindow )
		pWindow->SetVisible( !pWindow->IsVisible() );

	return std::string();
}

void AddMainMenuItem( CWindow * pWindow, const char * pszLabel, const char * pszWindow )
{
	pWindow->SetHeight( pWindow->GetHeight() + 25 );
	
	CButton * pButton = new CButton( 0 );

	pButton->SetHeight( BUTTON_HEIGHT );
	pButton->SetWidth( 100 );
	pButton->SetRelPos( CPos( 15, g_iMenuSize * 25 + 10 ) );
	pButton->SetString( pszLabel );
	pButton->SetString( pszWindow, 1 );
	pButton->SetCallback( MainMenuCallback );

	pWindow->AddElement( pButton );

	g_iMenuSize++;
}

CWindow * CreateMainMenu()
{
	TiXmlElement * pElement = new TiXmlElement( "Window" );

	pElement->SetAttribute( "absX", 100 );
	pElement->SetAttribute( "absY", 330 );
	pElement->SetAttribute( "width", 130 );
	pElement->SetAttribute( "height", 40 );
	pElement->SetAttribute( "string", "Main menu" );
	pElement->SetAttribute( "string2", "WINDOW_MAIN" );
	pElement->SetAttribute( "visible", 0 );
	pElement->SetAttribute( "closebutton", 1 );
	g_iMenuSize = 0;

	CWindow * pWindow = gpGui->AddWindow( new CWindow( pElement ) );

	delete pElement;
	return pWindow;
}

void LoadGUI( IDirect3DDevice9 * pDevice )
{
	gpGui = new CGUI( pDevice );

	gpGui->LoadInterfaceFromFile( "ColorThemes.xml" );
	gpGui->LoadInterfaceFromFile( "GuiTest_UI.xml" );

	CWindow * pMainMenu = CreateMainMenu();

	AddMainMenuItem( pMainMenu, "Waypoints", "WINDOW_WAYPOINT_CONTROLS" );
	AddMainMenuItem( pMainMenu, "Test 2", "WINDOW_TEST2" );

	pMainMenu->SetVisible( true );

	gpGui->SetVisible( true );
}

/*DWORD WINAPI LoadWallpaper( LPVOID )
{
	CoInitialize( 0 );

	IActiveDesktop* pActiveDesktop = 0;
	if( !FAILED( CoCreateInstance( CLSID_ActiveDesktop, 0, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (void**) &pActiveDesktop ) ) )
	{
	  #ifndef AD_GETWP_BMP
	  #define AD_GETWP_BMP 0x00000000
	  #endif
	  
		wchar_t * pwszStr = new wchar_t[ MAX_PATH ];
		pActiveDesktop->GetWallpaper( pwszStr, MAX_PATH, AD_GETWP_BMP );

		std::wstring wString( pwszStr );

		pBackground = new CTexture( gpGui->GetSprite(), std::string( wString.begin(), wString.end() ).c_str() );

		delete pwszStr;
	}

	CoUninitialize();
	return 0;
}*/

bool bReset = false;

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( gpGui )
		if( gpGui->GetMouse().HandleMessage( uMsg, wParam, lParam ) || gpGui->GetKeyboard()->HandleMessage( uMsg, wParam, lParam ) )
			return 0;

	switch( uMsg )
	{
		case WM_DESTROY:
		case WM_CLOSE:
			{
			   DestroyWindow( hWnd );
			   UnregisterClassA( "WoWXGui", GetModuleHandle( 0 ) );
			   g_bRunning = false;
			   return 0;
			}
		case WM_KEYDOWN:
			{
				if( GetAsyncKeyState( VK_UP )&1 )
				{
					if( gpGui )
					{
						IDirect3DDevice9 * pDevice = gpGui->GetDevice();
						
						SAFE_DELETE( gpGui );
						LoadGUI( pDevice );
						
						//pBackground->SetSprite( gpGui->GetSprite() );
					}
				}
				else if( GetAsyncKeyState( VK_DOWN )&1 )
				{
					IDirect3DDevice9 * pDevice = gpGui->GetDevice();

					bReset = true;
					do
					{
						gpGui->OnLostDevice();
						pDevice->Reset( &d3dPP );
						gpGui->OnResetDevice( pDevice );
					}
					while( FAILED( pDevice->TestCooperativeLevel() ) );
					bReset = false;
				}
			}
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

bool InitWindowClass( HINSTANCE hInstance )
{
	WNDCLASS wndClass;
	ZeroMemory( &wndClass, sizeof( WNDCLASS ) );

	wndClass.style			= CS_OWNDC;
	wndClass.hInstance		= hInstance;
	wndClass.hIcon			= LoadIcon( NULL, IDI_APPLICATION );
	wndClass.hCursor		= LoadCursor( NULL, IDC_ARROW );
	wndClass.hbrBackground	= (HBRUSH)GetStockObject( BLACK_BRUSH );
	wndClass.lpszClassName	= L"WoWXGui";
	wndClass.lpfnWndProc	= WndProc;

	if( !RegisterClass( &wndClass ) )
		return false;

	return true;
}

void InitDevice( IDirect3D9 * pD3D, HWND hWindow, IDirect3DDevice9 ** ppDevice )
{
	ZeroMemory( &d3dPP, sizeof( D3DPRESENT_PARAMETERS ) );

	d3dPP.BackBufferCount = 1;
	d3dPP.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dPP.hDeviceWindow = hWindow;
	d3dPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dPP.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dPP.BackBufferFormat = D3DFMT_R5G6B5;
	d3dPP.Windowed = TRUE;

	pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dPP, ppDevice );
}

void MessageHandler()
{
	MSG Msg = { 0 };
	while( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) )
	{
		TranslateMessage( &Msg );
		DispatchMessage( &Msg );

		ZeroMemory( &Msg, sizeof( MSG ) );
	}
}

HWND hWindow = 0;
const int iWidth = 800, iHeight = 600;

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int )
{
	if( !InitWindowClass( hInstance ) )
		return -1;

	hWindow = CreateWindowA( "WoWXGui", "WoWXGui test environment", WS_OVERLAPPED|WS_SYSMENU|WS_VISIBLE|WS_MINIMIZEBOX|WS_MAXIMIZEBOX, 0, 0, iWidth, iHeight, 0, 0, hInstance, 0 );

	if( !hWindow )
		return -2;

	RECT wndRect = { 0 };
	GetClientRect( hWindow,&wndRect );
    MoveWindow( hWindow, 0, 0, iWidth + ( iWidth - wndRect.right ), iHeight + ( iHeight - wndRect.bottom ), 1 );

	IDirect3D9 * pD3D = Direct3DCreate9( D3D_SDK_VERSION );

	if( !pD3D )
		return -4;

	InitDevice( pD3D, hWindow, &g_pDevice );

	if( !g_pDevice )
		return -5;

	LoadGUI( g_pDevice );
	//CreateThread( 0, 0, LoadWallpaper, 0, 0, 0 );

	while( g_bRunning )
	{
		if( GetAsyncKeyState( VK_ESCAPE ) && GetForegroundWindow() == hWindow )
			break;

		MessageHandler();
		RenderScreen();
	}

	return 0;
}

int iFrames = 0;
DWORD dwTimer = 0;

void RenderScreen()
{
	if( GetTickCount() - dwTimer >= 1000 )
	{
	  std::stringstream szBuffer;
	  szBuffer << "WoWXGui test environment FPS: " << iFrames;
		SetWindowTextA( hWindow, szBuffer.str().c_str() );

		iFrames = 0;
		dwTimer = GetTickCount();
	}

	HRESULT hCoopLevel = g_pDevice->TestCooperativeLevel();

	if( !FAILED( hCoopLevel ) && !bReset )
	{
		g_pDevice->Clear( 0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB( 0, 0, 0 ), 1.0f, 0 );
		g_pDevice->BeginScene();

		/*if( pBackground )
			pBackground->Draw( CPos( 0, 0 ), iWidth, iHeight );*/

		if( gpGui )
			gpGui->Draw();

		g_pDevice->EndScene();
		g_pDevice->Present( 0, 0, 0, 0 );

		iFrames++;
	}
}
