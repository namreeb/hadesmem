#pragma once

#include <Windows.h>
#include <memory>
#include <d3dx10.h>

#include <boost\format.hpp>

namespace boost
{
#ifdef UNICODE
	typedef boost::wformat tformat;
#else
	typedef boost::format tformat;
#endif
};

namespace JustConsole
{
	namespace Renderer
	{
		class IRenderer
		{
		public:
			typedef std::shared_ptr<IRenderer> RendererPtr;

			virtual ~IRenderer(){};

			virtual void FillArea( int x, int y, int Width, int Height,
				D3DXCOLOR Colour ) = 0;

			virtual void DrawText( int x, int y, const std::wstring& text,
				D3DXCOLOR Colour ) = 0;
				
			virtual void DrawText( int x, int y, const boost::tformat& fmt,
				D3DXCOLOR Colour ) = 0;

			virtual int GetTextWidth( const std::wstring& Text ) = 0;
			  
			virtual int GetCharWidth( wchar_t Char ) = 0;
		};
	}
}