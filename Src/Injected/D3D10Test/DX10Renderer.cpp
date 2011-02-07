#include "DX10Renderer.h"
#include "Locale.h"

#include <iostream>
#include <exception>

namespace JustConsole
{
	namespace Renderer
	{
		DX10Renderer::DX10Renderer( struct ID3D10Device* Device )
			: m_Device(Device), m_InputLayout(nullptr),
			m_VertexBuffer(nullptr)
		{
			D3D10_BUFFER_DESC BufferDesc;
			BufferDesc.Usage				= D3D10_USAGE_DYNAMIC;
			BufferDesc.ByteWidth			= sizeof(SimpleVertex) * 4;
			BufferDesc.BindFlags			= D3D10_BIND_VERTEX_BUFFER;
			BufferDesc.CPUAccessFlags		= D3D10_CPU_ACCESS_WRITE;
			BufferDesc.MiscFlags			= 0;

			std::cout << "Renderer :: Initializing" << std::endl;

			try
			{
				if ( FAILED( Device->CreateBuffer(&BufferDesc, nullptr,
					&m_VertexBuffer) ) )
					throw std::runtime_error(
						"Failed to create Vertex Buffer" );

				if ( FAILED( D3DX10CreateEffectFromFile(L"Effect.fx", 0, 0,
					"fx_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
					m_Device, 0, 0, &m_Effect, 0, 0) ) )
					throw std::runtime_error( "Failed to create Effect" );

				m_Technique = m_Effect->GetTechniqueByName( "Render" );

				D3D10_VIEWPORT Viewport;
				UINT NumViewports = 1;
				m_Device->RSGetViewports( &NumViewports, &Viewport );

				float Data[] = { static_cast<float>(Viewport.Width), 
					static_cast<float>(Viewport.Height) };
				m_Effect->GetVariableByName("Viewport")->AsVector()->
					SetFloatVector(	Data );

				D3D10_INPUT_ELEMENT_DESC Layout[] =
				{
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0,
					D3D10_INPUT_PER_VERTEX_DATA, 0 }, 
					{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0,
					12,	D3D10_INPUT_PER_VERTEX_DATA, 0 },
				};

				D3D10_PASS_DESC PassDesc;
				m_Technique->GetPassByIndex(0)->GetDesc(&PassDesc);
				m_Device->CreateInputLayout( Layout, 2,
					PassDesc.pIAInputSignature,
					PassDesc.IAInputSignatureSize, &m_InputLayout );

				D3D10_RASTERIZER_DESC rasterizerState;
				rasterizerState.CullMode = D3D10_CULL_NONE;
				rasterizerState.FillMode = D3D10_FILL_SOLID;
				rasterizerState.FrontCounterClockwise = true;
				rasterizerState.DepthBias = false;
				rasterizerState.DepthBiasClamp = 0;
				rasterizerState.SlopeScaledDepthBias = 0;
				rasterizerState.DepthClipEnable = true;
				rasterizerState.ScissorEnable = false;
				rasterizerState.MultisampleEnable = true;
				rasterizerState.AntialiasedLineEnable = true;

				m_Device->CreateRasterizerState( &rasterizerState,
					&m_RasterizerState);
				m_Device->RSSetState(m_RasterizerState);

				D3D10_BLEND_DESC BlendState;
				memset(&BlendState, 0, sizeof(D3D10_BLEND_DESC));
				BlendState.BlendEnable[0] = true;
				BlendState.RenderTargetWriteMask[0] =
					D3D10_COLOR_WRITE_ENABLE_ALL;
				BlendState.SrcBlend = D3D10_BLEND_SRC_ALPHA;
				BlendState.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
				BlendState.BlendOp = D3D10_BLEND_OP_ADD;
				BlendState.SrcBlendAlpha = D3D10_BLEND_ONE;
				BlendState.DestBlendAlpha = D3D10_BLEND_ZERO;
				BlendState.BlendOpAlpha = D3D10_BLEND_OP_ADD;

				m_Device->CreateBlendState( &BlendState, &m_BlendState );
				m_Device->OMSetBlendState( m_BlendState, 0, 0xFFFFFFFF );

				if ( FAILED( D3DX10CreateFont( m_Device, 12, 0, FW_NORMAL,
					1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
					DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, L"Arial",
					&m_Font ) ) )
					throw std::runtime_error( "Failed to create Font" );

				if ( FAILED( D3DX10CreateSprite( m_Device, 512,
					&m_FontSprite ) ) )
					throw std::runtime_error(
					"Failed to create Font sprite" );

				std::cout << "Renderer :: Initialize complete" << std::endl;
			}
			catch( const std::exception& e )
			{
				std::cout << "Renderer :: Failed to initialize: " <<
					e.what() <<	std::endl;
			}
		}

		DX10Renderer::~DX10Renderer()
		{
			std::cout << "Renderer :: Shutdown" << std::endl;
		};

		void DX10Renderer::FillArea( int x, int y, int Width, int Height,
			D3DXCOLOR Colour )
		{
			SimpleVertex* Data = 0;

			Colour /= 255;

			SimpleVertex Vertices[] = {
				{ D3DXVECTOR3((FLOAT)x,			(FLOAT)(y + Height), 0),	Colour },
				{ D3DXVECTOR3((FLOAT)x,			(FLOAT)y, 0),			Colour },
				{ D3DXVECTOR3((FLOAT)(x + Width),	(FLOAT)(y + Height), 0),	Colour },
				{ D3DXVECTOR3((FLOAT)(x + Width),	(FLOAT)y, 0), 			Colour }
			};

			HRESULT hr = m_VertexBuffer->Map( D3D10_MAP_WRITE_DISCARD, 0,
				reinterpret_cast<void**>(&Data) );
			if ( SUCCEEDED( hr ) )
			{
				memcpy_s( Data, sizeof( Vertices ), Vertices,
					sizeof( Vertices ) );
				m_VertexBuffer->Unmap();
			} else
				return;

			UINT Stride = sizeof(SimpleVertex);
			UINT Offset = 0;
			m_Device->IASetInputLayout( m_InputLayout );
			m_Device->IASetVertexBuffers( 0, 1, &m_VertexBuffer.p, &Stride,
				&Offset );
			m_Device->IASetPrimitiveTopology(
				D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			D3D10_TECHNIQUE_DESC techDesc;
			m_Technique->GetDesc( &techDesc );
			for( UINT p = 0; p < techDesc.Passes; ++p )
			{
				m_Technique->GetPassByIndex( p )->Apply( 0 );
				m_Device->Draw( 4, 0 );
			}
		}

		void DX10Renderer::DrawText( int x, int y, const std::wstring& text,
			D3DXCOLOR Colour )
		{
			RECT rect = { x, y, 0, 0 };
			m_FontSprite->Begin( D3DX10_SPRITE_SORT_TEXTURE );
			m_Font->DrawTextW( m_FontSprite, text.c_str(), text.length(),
				&rect, DT_NOCLIP|DT_LEFT,
				Colour );
			m_FontSprite->End();
		}

		void DX10Renderer::DrawText( int x, int y,
			const boost::tformat& fmt, D3DXCOLOR Colour )
		{
			DrawText( x, y, fmt.str(), Colour );
		}

		int DX10Renderer::GetTextWidth( const std::wstring& Text )
		{
			RECT rect = { 0, 0, 0, 0 };
			m_Font->DrawTextW( 0, Text.c_str(), Text.length(), &rect,
				DT_CALCRECT,
				D3DXCOLOR() );

			return rect.right;
		}

		int DX10Renderer::GetCharWidth( wchar_t Char )
		{
			RECT rect = { 0, 0, 0, 0 };
			m_Font->DrawTextW( 0, &Char, 1, &rect, DT_LEFT|DT_CALCRECT,
				D3DXCOLOR() );

			return rect.right;
		}
	}
}