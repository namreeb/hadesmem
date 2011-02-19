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

// DirectX
#define D3D11_IGNORE_SDK_LAYERS
#include <d3d11.h>

// Hades
#include "Renderer.hpp"
#include "HadesKernel/Kernel.hpp"

namespace Hades
{
  namespace D3D11
  {
    class ID3D11DeviceHook : public ID3D11Device
    {
    public:
      ID3D11DeviceHook(Kernel::Kernel& MyKernel, 
        ID3D11Device* pDevice, 
        ID3D11DeviceContext* pDeviceContext) 
        : m_Kernel(MyKernel), 
        m_pDeviceContext(pDeviceContext), 
        m_pDevice(pDevice), 
        m_pRenderer(new GUI::D3D11Renderer(pDevice, pDeviceContext))
      { }
        
      ID3D11DeviceHook& operator=(ID3D11DeviceHook const&);
      
      void OnFrame() 
      {
        m_Kernel.OnFrame(*m_pRenderer);
      }
      
      // 
      
      virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid, 
        void** ppvObject);
        
      virtual ULONG STDMETHODCALLTYPE AddRef();
        
      virtual ULONG STDMETHODCALLTYPE Release();
      
      // 
      
      virtual HRESULT STDMETHODCALLTYPE CreateBuffer( 
        const D3D11_BUFFER_DESC *pDesc,
        const D3D11_SUBRESOURCE_DATA *pInitialData,
        ID3D11Buffer **ppBuffer);
        
      virtual HRESULT STDMETHODCALLTYPE CreateTexture1D( 
        const D3D11_TEXTURE1D_DESC *pDesc,
        const D3D11_SUBRESOURCE_DATA *pInitialData,
        ID3D11Texture1D **ppTexture1D);

      virtual HRESULT STDMETHODCALLTYPE CreateTexture2D( 
        const D3D11_TEXTURE2D_DESC *pDesc,
        const D3D11_SUBRESOURCE_DATA *pInitialData,
        ID3D11Texture2D **ppTexture2D);

      virtual HRESULT STDMETHODCALLTYPE CreateTexture3D( 
        const D3D11_TEXTURE3D_DESC *pDesc,
        const D3D11_SUBRESOURCE_DATA *pInitialData,
        ID3D11Texture3D **ppTexture3D);

      virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView( 
        ID3D11Resource *pResource,
        const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
        ID3D11ShaderResourceView **ppSRView);

      virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView( 
        ID3D11Resource *pResource,
        const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
        ID3D11UnorderedAccessView **ppUAView);

      virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView( 
        ID3D11Resource *pResource,
        const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
        ID3D11RenderTargetView **ppRTView);

      virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView( 
        ID3D11Resource *pResource,
        const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
        ID3D11DepthStencilView **ppDepthStencilView);

      virtual HRESULT STDMETHODCALLTYPE CreateInputLayout( 
        const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
        UINT NumElements,
        const void *pShaderBytecodeWithInputSignature,
        SIZE_T BytecodeLength,
        ID3D11InputLayout **ppInputLayout);

      virtual HRESULT STDMETHODCALLTYPE CreateVertexShader( 
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        ID3D11ClassLinkage *pClassLinkage,
        ID3D11VertexShader **ppVertexShader);

      virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader( 
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        ID3D11ClassLinkage *pClassLinkage,
        ID3D11GeometryShader **ppGeometryShader);

      virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput( 
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
        UINT NumEntries,
        const UINT *pBufferStrides,
        UINT NumStrides,
        UINT RasterizedStream,
        ID3D11ClassLinkage *pClassLinkage,
        ID3D11GeometryShader **ppGeometryShader);

      virtual HRESULT STDMETHODCALLTYPE CreatePixelShader( 
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        ID3D11ClassLinkage *pClassLinkage,
        ID3D11PixelShader **ppPixelShader);

      virtual HRESULT STDMETHODCALLTYPE CreateHullShader( 
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        ID3D11ClassLinkage *pClassLinkage,
        ID3D11HullShader **ppHullShader);

      virtual HRESULT STDMETHODCALLTYPE CreateDomainShader( 
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        ID3D11ClassLinkage *pClassLinkage,
        ID3D11DomainShader **ppDomainShader);

      virtual HRESULT STDMETHODCALLTYPE CreateComputeShader( 
        const void *pShaderBytecode,
        SIZE_T BytecodeLength,
        ID3D11ClassLinkage *pClassLinkage,
        ID3D11ComputeShader **ppComputeShader);

      virtual HRESULT STDMETHODCALLTYPE CreateClassLinkage( 
        ID3D11ClassLinkage **ppLinkage);

      virtual HRESULT STDMETHODCALLTYPE CreateBlendState( 
        const D3D11_BLEND_DESC *pBlendStateDesc,
        ID3D11BlendState **ppBlendState);

      virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState( 
        const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
        ID3D11DepthStencilState **ppDepthStencilState);

      virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState( 
        const D3D11_RASTERIZER_DESC *pRasterizerDesc,
        ID3D11RasterizerState **ppRasterizerState);

      virtual HRESULT STDMETHODCALLTYPE CreateSamplerState( 
        const D3D11_SAMPLER_DESC *pSamplerDesc,
        ID3D11SamplerState **ppSamplerState);

      virtual HRESULT STDMETHODCALLTYPE CreateQuery( 
        const D3D11_QUERY_DESC *pQueryDesc,
        ID3D11Query **ppQuery);

      virtual HRESULT STDMETHODCALLTYPE CreatePredicate( 
        const D3D11_QUERY_DESC *pPredicateDesc,
        ID3D11Predicate **ppPredicate);

      virtual HRESULT STDMETHODCALLTYPE CreateCounter( 
        const D3D11_COUNTER_DESC *pCounterDesc,
        ID3D11Counter **ppCounter);

      virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext( 
        UINT ContextFlags,
        ID3D11DeviceContext **ppDeferredContext);

      virtual HRESULT STDMETHODCALLTYPE OpenSharedResource( 
        HANDLE hResource,
        REFIID ReturnedInterface,
        void **ppResource);

      virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport( 
        DXGI_FORMAT Format,
        UINT *pFormatSupport);

      virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels( 
        DXGI_FORMAT Format,
        UINT SampleCount,
        UINT *pNumQualityLevels);

      virtual void STDMETHODCALLTYPE CheckCounterInfo( 
        D3D11_COUNTER_INFO *pCounterInfo);

      virtual HRESULT STDMETHODCALLTYPE CheckCounter( 
        const D3D11_COUNTER_DESC *pDesc,
        D3D11_COUNTER_TYPE *pType,
        UINT *pActiveCounters,
        LPSTR szName,
        UINT *pNameLength,
        LPSTR szUnits,
        UINT *pUnitsLength,
        LPSTR szDescription,
        UINT *pDescriptionLength);

      virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport( 
        D3D11_FEATURE Feature,
        void *pFeatureSupportData,
        UINT FeatureSupportDataSize);

      virtual HRESULT STDMETHODCALLTYPE GetPrivateData( 
        REFGUID guid,
        UINT *pDataSize,
        void *pData);

      virtual HRESULT STDMETHODCALLTYPE SetPrivateData( 
        REFGUID guid,
        UINT DataSize,
        const void *pData);

      virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface( 
        REFGUID guid,
        const IUnknown *pData);

      virtual D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel();

      virtual UINT STDMETHODCALLTYPE GetCreationFlags();

      virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason();

      virtual void STDMETHODCALLTYPE GetImmediateContext( 
        ID3D11DeviceContext **ppImmediateContext);

      virtual HRESULT STDMETHODCALLTYPE SetExceptionMode( 
        UINT RaiseFlags);

      virtual UINT STDMETHODCALLTYPE GetExceptionMode();
      
      // 
      
    private:
      Kernel::Kernel& m_Kernel;
  		ID3D11DeviceContext* m_pDeviceContext;
  		ID3D11Device* m_pDevice;
  		std::shared_ptr<GUI::D3D11Renderer> m_pRenderer;
    };
  }
}
