/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// Windows API
#include <Windows.h>
#include <atlbase.h>

// DirectX
#define D3D11_IGNORE_SDK_LAYERS
#include <d3d11.h>
#include <d3dx11.h>
#include <xnamath.h>
#include <d3dcompiler.h>

// Hades
#include "HadesRenderer/Renderer.hpp"

namespace Hades
{
  namespace GUI
  {
    class D3D11Renderer : public Renderer
    {
    public:
      explicit D3D11Renderer(ID3D11Device* pDevice, 
      ID3D11DeviceContext* pDeviceContext) 
        : m_pDevice(pDevice), 
        m_pDeviceContext(pDeviceContext), 
        m_pVertexShader(nullptr), 
        m_pPixelShader(nullptr), 
        m_pVertexLayout(nullptr), 
        m_pVertexBuffer(nullptr)
      {
        char const Tutorial02Fx[] = 
        "//--------------------------------------------------------------------------------------\n"
        "// File: Tutorial02.fx\n"
        "//\n"
        "// Copyright (c) Microsoft Corporation. All rights reserved.\n"
        "//--------------------------------------------------------------------------------------\n"
        "\n"
        "//--------------------------------------------------------------------------------------\n"
        "// Vertex Shader\n"
        "//--------------------------------------------------------------------------------------\n"
        "float4 VS( float4 Pos : POSITION ) : SV_POSITION\n"
        "{\n"
        "    return Pos;\n"
        "}\n"
        "\n"
        "\n"
        "//--------------------------------------------------------------------------------------\n"
        "// Pixel Shader\n"
        "//--------------------------------------------------------------------------------------\n"
        "float4 PS( float4 Pos : SV_POSITION ) : SV_Target\n"
        "{\n"
        "    return float4( 0.0f, 1.0f, 0.0f, 1.0f );    // Green, with Alpha = 1\n"
        "}\n"
        ;
        
        // Compile the vertex shader
        ID3DBlob* pVSBlob = NULL;
        HRESULT hr = CompileShaderFromMemory(Tutorial02Fx, sizeof(Tutorial02Fx), 
          "Tutorial02.fx", "VS", "vs_4_0", &pVSBlob );
        if( FAILED( hr ) )
        {
            MessageBox( NULL, L"The FX file cannot be compiled.", L"Error", 
              MB_OK );
            return; //hr;
        }

        // Create the vertex shader
        hr = m_pDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), 
          pVSBlob->GetBufferSize(), NULL, &m_pVertexShader );
        if( FAILED( hr ) )
        {	
          MessageBox( NULL, L"Could not create vertex shader.", L"Error", 
            MB_OK );
          pVSBlob->Release();
          return; //hr;
        }
        
        // Define the input layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
          {
            { 
              "POSITION", 
              0, 
              DXGI_FORMAT_R32G32B32_FLOAT, 
              0, 
              0, 
              D3D11_INPUT_PER_VERTEX_DATA, 
              0 
            },
          };
        UINT numElements = ARRAYSIZE( layout );
        
        // Create the input layout
        hr = m_pDevice->CreateInputLayout( layout, numElements, 
          pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), 
          &m_pVertexLayout );
        pVSBlob->Release();
        if( FAILED( hr ) )
        {
          MessageBox( NULL, L"Could not create input layout.", L"Error", 
            MB_OK );
          return; //hr;
        }
        
        // Set the input layout
        m_pDeviceContext->IASetInputLayout( m_pVertexLayout );
        
        // Compile the pixel shader
        ID3DBlob* pPSBlob = NULL;
        hr = CompileShaderFromMemory(Tutorial02Fx, sizeof(Tutorial02Fx), 
          "Tutorial02.fx", "PS", "ps_4_0", &pPSBlob );
        if( FAILED( hr ) )
        {
          MessageBox( NULL, L"The FX file cannot be compiled.", L"Error", 
            MB_OK );
          return; //hr;
        }
        
        // Create the pixel shader
        hr = m_pDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), 
          pPSBlob->GetBufferSize(), NULL, &m_pPixelShader );
        pPSBlob->Release();
        if( FAILED( hr ) )
        {
          MessageBox( NULL, L"Could not create pixel shader.", L"Error", 
            MB_OK );
          return; //hr;
        }
        
        // Create vertex buffer
        struct SimpleVertex
        {
          XMFLOAT3 Pos;
        };
        SimpleVertex vertices[] =
        {
          XMFLOAT3( 0.0f, 0.5f, 0.5f ),
          XMFLOAT3( 0.5f, -0.5f, 0.5f ),
          XMFLOAT3( -0.5f, -0.5f, 0.5f ),
        };
        D3D11_BUFFER_DESC bd;
        ZeroMemory( &bd, sizeof(bd) );
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof( SimpleVertex ) * 3;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA InitData;
        ZeroMemory( &InitData, sizeof(InitData) );
        InitData.pSysMem = vertices;
        hr = m_pDevice->CreateBuffer( &bd, &InitData, &m_pVertexBuffer );
        if( FAILED( hr ) )
        {
          MessageBox( NULL, L"Could not create vertex buffer.", L"Error", 
            MB_OK );
          return; //hr;
        }
        
        // Set vertex buffer
        UINT stride = sizeof( SimpleVertex );
        UINT offset = 0;
        m_pDeviceContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );
        
        // Set primitive topology
        m_pDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
      }
      
      void PreReset()
      {
      }
      
      void PostReset()
      {
      }
      
      void DrawText(std::wstring const& /*Text*/, unsigned int /*X*/, unsigned int /*Y*/)
      {
        // Todo: Actually draw text rather than some test stuff...
        
        // Render a triangle
        m_pDeviceContext->VSSetShader( m_pVertexShader, NULL, 0 );
        m_pDeviceContext->PSSetShader( m_pPixelShader, NULL, 0 );
        m_pDeviceContext->Draw( 3, 0 );
      }
      
      HRESULT CompileShaderFromMemory(LPCSTR pFileBuf, SIZE_T FileSize, LPCSTR szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
      {
          HRESULT hr = S_OK;
      
          DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
      
          ID3DBlob* pErrorBlob;
          hr = D3DX11CompileFromMemory( pFileBuf, FileSize, szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
              dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
          if( FAILED(hr) )
          {
              if( pErrorBlob != NULL )
                  OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
              if( pErrorBlob ) pErrorBlob->Release();
                return hr;
          }
          if( pErrorBlob ) pErrorBlob->Release();
      
          return S_OK;
      }

      
    private:
      ID3D11Device* m_pDevice;
      ID3D11DeviceContext* m_pDeviceContext;
      
      ID3D11VertexShader* m_pVertexShader;
      ID3D11PixelShader* m_pPixelShader;
      ID3D11InputLayout* m_pVertexLayout;
      ID3D11Buffer* m_pVertexBuffer;
    };
  }
}
