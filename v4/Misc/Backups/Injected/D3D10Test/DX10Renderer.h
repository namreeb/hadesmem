#pragma once

#include "IRenderer.h"

#include <d3dx10.h>
#include <atlbase.h>

namespace JustConsole
{
	namespace Renderer
	{
		class DX10Renderer : public IRenderer
		{
		private:
			struct SimpleVertex
			{
				D3DXVECTOR3	Pos;
				D3DXVECTOR4	Color;
			};

		public:
			DX10Renderer( ID3D10Device* Device );
			~DX10Renderer();

			virtual void FillArea( int x, int y, int Width, int Height,
				D3DXCOLOR Colour );
				
			virtual void DrawText( int x, int y, const std::wstring& text,
				D3DXCOLOR Colour );
				
			virtual void DrawText( int x, int y, const boost::tformat& fmt,
				D3DXCOLOR Colour );
				
			virtual int GetTextWidth( const std::wstring& Text );
			  
			virtual int GetCharWidth( wchar_t Char );

		private:
			ID3D10Device*						m_Device;
			CComPtr<ID3D10Buffer>				m_VertexBuffer;
			CComPtr<ID3D10InputLayout>			m_InputLayout;
			CComPtr<ID3D10Effect>				m_Effect;
			CComPtr<ID3D10BlendState>			m_BlendState;
			CComPtr<ID3D10RasterizerState>		m_RasterizerState;
			CComPtr<ID3DX10Font>				m_Font;
			CComPtr<ID3DX10Sprite>				m_FontSprite;
			ID3D10EffectTechnique*				m_Technique;
		};
	}
}