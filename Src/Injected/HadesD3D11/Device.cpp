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

// Hades
#include "Device.hpp"
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace D3D11
  {
    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::QueryInterface(
      REFIID riid, 
      void** ppvObject)
    {
      return m_pDevice->QueryInterface(riid, ppvObject);
    }
      
    ULONG STDMETHODCALLTYPE ID3D11DeviceHook::AddRef()
    {
      return m_pDevice->AddRef();
    }
      
    ULONG STDMETHODCALLTYPE ID3D11DeviceHook::Release()
    {
      return m_pDevice->Release();
    }
     
    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateBuffer( 
      const D3D11_BUFFER_DESC *pDesc,
      const D3D11_SUBRESOURCE_DATA *pInitialData,
      ID3D11Buffer **ppBuffer)
    {
      return m_pDevice->CreateBuffer(pDesc, pInitialData, ppBuffer);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateTexture1D( 
      const D3D11_TEXTURE1D_DESC *pDesc,
      const D3D11_SUBRESOURCE_DATA *pInitialData,
      ID3D11Texture1D **ppTexture1D)
    {
      return m_pDevice->CreateTexture1D(pDesc, pInitialData, ppTexture1D);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateTexture2D( 
      const D3D11_TEXTURE2D_DESC *pDesc,
      const D3D11_SUBRESOURCE_DATA *pInitialData,
      ID3D11Texture2D **ppTexture2D)
    {
      return m_pDevice->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateTexture3D( 
      const D3D11_TEXTURE3D_DESC *pDesc,
      const D3D11_SUBRESOURCE_DATA *pInitialData,
      ID3D11Texture3D **ppTexture3D)
    {
      return m_pDevice->CreateTexture3D(pDesc, pInitialData, ppTexture3D);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateShaderResourceView( 
      ID3D11Resource *pResource,
      const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
      ID3D11ShaderResourceView **ppSRView)
    {
      return m_pDevice->CreateShaderResourceView(pResource, pDesc, ppSRView);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateUnorderedAccessView( 
      ID3D11Resource *pResource,
      const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
      ID3D11UnorderedAccessView **ppUAView)
    {
      return m_pDevice->CreateUnorderedAccessView(pResource, pDesc, ppUAView);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateRenderTargetView( 
      ID3D11Resource *pResource,
      const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
      ID3D11RenderTargetView **ppRTView)
    {
      return m_pDevice->CreateRenderTargetView(pResource, pDesc, ppRTView);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateDepthStencilView( 
      ID3D11Resource *pResource,
      const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
      ID3D11DepthStencilView **ppDepthStencilView)
    {
      return m_pDevice->CreateDepthStencilView(pResource, pDesc, 
        ppDepthStencilView);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateInputLayout( 
      const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
      UINT NumElements,
      const void *pShaderBytecodeWithInputSignature,
      SIZE_T BytecodeLength,
      ID3D11InputLayout **ppInputLayout)
    {
      return m_pDevice->CreateInputLayout(pInputElementDescs, NumElements, 
        pShaderBytecodeWithInputSignature, BytecodeLength, ppInputLayout);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateVertexShader( 
      const void *pShaderBytecode,
      SIZE_T BytecodeLength,
      ID3D11ClassLinkage *pClassLinkage,
      ID3D11VertexShader **ppVertexShader)
    {
      return m_pDevice->CreateVertexShader(pShaderBytecode, BytecodeLength, 
        pClassLinkage, ppVertexShader);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateGeometryShader( 
      const void *pShaderBytecode,
      SIZE_T BytecodeLength,
      ID3D11ClassLinkage *pClassLinkage,
      ID3D11GeometryShader **ppGeometryShader)
    {
      return m_pDevice->CreateGeometryShader(pShaderBytecode, BytecodeLength, 
        pClassLinkage, ppGeometryShader);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateGeometryShaderWithStreamOutput( 
      const void *pShaderBytecode,
      SIZE_T BytecodeLength,
      const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
      UINT NumEntries,
      const UINT *pBufferStrides,
      UINT NumStrides,
      UINT RasterizedStream,
      ID3D11ClassLinkage *pClassLinkage,
      ID3D11GeometryShader **ppGeometryShader)
    {
      return m_pDevice->CreateGeometryShaderWithStreamOutput(pShaderBytecode, 
        BytecodeLength, pSODeclaration, NumEntries, pBufferStrides, 
        NumStrides, RasterizedStream, pClassLinkage, ppGeometryShader);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreatePixelShader( 
      const void *pShaderBytecode,
      SIZE_T BytecodeLength,
      ID3D11ClassLinkage *pClassLinkage,
      ID3D11PixelShader **ppPixelShader)
    {
      return m_pDevice->CreatePixelShader(pShaderBytecode, BytecodeLength, 
        pClassLinkage, ppPixelShader);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateHullShader( 
      const void *pShaderBytecode,
      SIZE_T BytecodeLength,
      ID3D11ClassLinkage *pClassLinkage,
      ID3D11HullShader **ppHullShader)
    {
      return m_pDevice->CreateHullShader(pShaderBytecode, BytecodeLength, 
        pClassLinkage, ppHullShader);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateDomainShader( 
      const void *pShaderBytecode,
      SIZE_T BytecodeLength,
      ID3D11ClassLinkage *pClassLinkage,
      ID3D11DomainShader **ppDomainShader)
    {
      return m_pDevice->CreateDomainShader(pShaderBytecode, BytecodeLength, 
        pClassLinkage, ppDomainShader);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateComputeShader( 
      const void *pShaderBytecode,
      SIZE_T BytecodeLength,
      ID3D11ClassLinkage *pClassLinkage,
      ID3D11ComputeShader **ppComputeShader)
    {
      return m_pDevice->CreateComputeShader(pShaderBytecode, BytecodeLength, 
        pClassLinkage, ppComputeShader);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateClassLinkage( 
      ID3D11ClassLinkage **ppLinkage)
    {
      return m_pDevice->CreateClassLinkage(ppLinkage);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateBlendState( 
      const D3D11_BLEND_DESC *pBlendStateDesc,
      ID3D11BlendState **ppBlendState)
    {
      return m_pDevice->CreateBlendState(pBlendStateDesc, ppBlendState);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateDepthStencilState( 
      const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc,
      ID3D11DepthStencilState **ppDepthStencilState)
    {
      return m_pDevice->CreateDepthStencilState(pDepthStencilDesc, 
        ppDepthStencilState);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateRasterizerState( 
      const D3D11_RASTERIZER_DESC *pRasterizerDesc,
      ID3D11RasterizerState **ppRasterizerState)
    {
      return m_pDevice->CreateRasterizerState(pRasterizerDesc, 
        ppRasterizerState);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateSamplerState( 
      const D3D11_SAMPLER_DESC *pSamplerDesc,
      ID3D11SamplerState **ppSamplerState)
    {
      return m_pDevice->CreateSamplerState(pSamplerDesc, ppSamplerState);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateQuery( 
      const D3D11_QUERY_DESC *pQueryDesc,
      ID3D11Query **ppQuery)
    {
      return m_pDevice->CreateQuery(pQueryDesc, ppQuery);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreatePredicate( 
      const D3D11_QUERY_DESC *pPredicateDesc,
      ID3D11Predicate **ppPredicate)
    {
      return m_pDevice->CreatePredicate(pPredicateDesc, ppPredicate);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateCounter( 
      const D3D11_COUNTER_DESC *pCounterDesc,
      ID3D11Counter **ppCounter)
    {
      return m_pDevice->CreateCounter(pCounterDesc, ppCounter);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CreateDeferredContext( 
      UINT ContextFlags,
      ID3D11DeviceContext **ppDeferredContext)
    {
      return m_pDevice->CreateDeferredContext(ContextFlags, ppDeferredContext);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::OpenSharedResource( 
      HANDLE hResource,
      REFIID ReturnedInterface,
      void **ppResource)
    {
      return m_pDevice->OpenSharedResource(hResource, ReturnedInterface, 
        ppResource);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CheckFormatSupport( 
      DXGI_FORMAT Format,
      UINT *pFormatSupport)
    {
      return m_pDevice->CheckFormatSupport(Format, pFormatSupport);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CheckMultisampleQualityLevels( 
      DXGI_FORMAT Format,
      UINT SampleCount,
      UINT *pNumQualityLevels)
    {
      return m_pDevice->CheckMultisampleQualityLevels(Format, SampleCount, 
        pNumQualityLevels);
    }

    void STDMETHODCALLTYPE ID3D11DeviceHook::CheckCounterInfo( 
      D3D11_COUNTER_INFO *pCounterInfo)
    {
      return m_pDevice->CheckCounterInfo(pCounterInfo);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CheckCounter( 
      const D3D11_COUNTER_DESC *pDesc,
      D3D11_COUNTER_TYPE *pType,
      UINT *pActiveCounters,
      LPSTR szName,
      UINT *pNameLength,
      LPSTR szUnits,
      UINT *pUnitsLength,
      LPSTR szDescription,
      UINT *pDescriptionLength)
    {
      return m_pDevice->CheckCounter(pDesc, pType, pActiveCounters, szName, 
        pNameLength, szUnits, pUnitsLength, szDescription, 
        pDescriptionLength);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::CheckFeatureSupport( 
      D3D11_FEATURE Feature,
      void *pFeatureSupportData,
      UINT FeatureSupportDataSize)
    {
      return m_pDevice->CheckFeatureSupport(Feature, pFeatureSupportData, 
        FeatureSupportDataSize);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::GetPrivateData( 
      REFGUID guid,
      UINT *pDataSize,
      void *pData)
    {
      return m_pDevice->GetPrivateData(guid, pDataSize, pData);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::SetPrivateData( 
      REFGUID guid,
      UINT DataSize,
      const void *pData)
    {
      return m_pDevice->SetPrivateData(guid, DataSize, pData);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::SetPrivateDataInterface( 
      REFGUID guid,
      const IUnknown *pData)
    {
      return m_pDevice->SetPrivateDataInterface(guid, pData);
    }

    D3D_FEATURE_LEVEL STDMETHODCALLTYPE ID3D11DeviceHook::GetFeatureLevel()
    {
      return m_pDevice->GetFeatureLevel();
    }

    UINT STDMETHODCALLTYPE ID3D11DeviceHook::GetCreationFlags()
    {
      return m_pDevice->GetCreationFlags();
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::GetDeviceRemovedReason()
    {
      return m_pDevice->GetDeviceRemovedReason();
    }

    void STDMETHODCALLTYPE ID3D11DeviceHook::GetImmediateContext( 
      ID3D11DeviceContext **ppImmediateContext)
    {
      return m_pDevice->GetImmediateContext(ppImmediateContext);
    }

    HRESULT STDMETHODCALLTYPE ID3D11DeviceHook::SetExceptionMode( 
      UINT RaiseFlags)
    {
      return m_pDevice->SetExceptionMode(RaiseFlags);
    }
    
    UINT STDMETHODCALLTYPE ID3D11DeviceHook::GetExceptionMode()
    {
      return m_pDevice->GetExceptionMode();
    }
  }
}

