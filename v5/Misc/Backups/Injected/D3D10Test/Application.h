#pragma once

#include <windows.h> 
#include <d3d10.h> 
#include <d3dx10.h>

#include <memory>

#include "DX10Renderer.h"

class Application
{
public:
	bool CreateD3DWindow(int width, int height);

	bool Initialize(); 
	void Render(float deltaTime);
	void MainLoop();

private:

	static LRESULT CALLBACK WndProc( HWND hWnd , UINT message , WPARAM wParam , LPARAM lParam);

	HWND						hWnd; 
	IDXGISwapChain*				pSwapChain;
	ID3D10RenderTargetView*		pRenderTargetView;
	ID3D10Device*				pDevice; 
	int							width, height;
	
	std::shared_ptr<JustConsole::Renderer::DX10Renderer> m_pRenderer;
};
