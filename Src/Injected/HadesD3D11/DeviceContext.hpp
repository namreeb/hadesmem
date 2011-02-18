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

// C++ Standard Library
#include <memory>

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
    class ID3D11DeviceContextHook : public ID3D11DeviceContext
    {
    public:
      ID3D11DeviceContextHook(Kernel::Kernel& MyKernel, 
        ID3D11DeviceContext* pDeviceContext, 
        ID3D11Device* pDevice) 
        : m_Kernel(MyKernel), 
        m_pDeviceContext(pDeviceContext), 
        m_pDevice(pDevice), 
        m_pRenderer(new GUI::D3D11Renderer(pDevice, pDeviceContext))
      { }
      
      // 
      
      virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid, 
        void** ppvObject);
        
      virtual ULONG STDMETHODCALLTYPE AddRef();
        
      virtual ULONG STDMETHODCALLTYPE Release();
      
      // 
      
      virtual void STDMETHODCALLTYPE GetDevice(
        ID3D11Device **ppDevice);
      
      virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
        REFGUID Name,
        UINT *pDataSize,
        void *pData);
      
      virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
        REFGUID Name,
        UINT DataSize,
        const void *pData);
      
      virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
        REFGUID Name,
        const IUnknown *pUnknown);
        
      // 
      
      virtual void STDMETHODCALLTYPE VSSetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE PSSetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE PSSetShader( 
        ID3D11PixelShader *pPixelShader,
        ID3D11ClassInstance *const *ppClassInstances,
        UINT NumClassInstances);
        
      virtual void STDMETHODCALLTYPE PSSetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState *const *ppSamplers);
        
      virtual void STDMETHODCALLTYPE VSSetShader( 
        ID3D11VertexShader *pVertexShader,
        ID3D11ClassInstance *const *ppClassInstances,
        UINT NumClassInstances);
        
      virtual void STDMETHODCALLTYPE DrawIndexed( 
        UINT IndexCount,
        UINT StartIndexLocation,
        INT BaseVertexLocation);
        
      virtual void STDMETHODCALLTYPE Draw( 
        UINT VertexCount,
        UINT StartVertexLocation);
        
      virtual HRESULT STDMETHODCALLTYPE Map( 
        ID3D11Resource *pResource,
        UINT Subresource,
        D3D11_MAP MapType,
        UINT MapFlags,
        D3D11_MAPPED_SUBRESOURCE *pMappedResource);
        
      virtual void STDMETHODCALLTYPE Unmap( 
        ID3D11Resource *pResource,
        UINT Subresource);
        
      virtual void STDMETHODCALLTYPE PSSetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE IASetInputLayout( 
        ID3D11InputLayout *pInputLayout);
        
      virtual void STDMETHODCALLTYPE IASetVertexBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppVertexBuffers,
        const UINT *pStrides,
        const UINT *pOffsets);
        
      virtual void STDMETHODCALLTYPE IASetIndexBuffer( 
        ID3D11Buffer *pIndexBuffer,
        DXGI_FORMAT Format,
        UINT Offset);
        
      virtual void STDMETHODCALLTYPE DrawIndexedInstanced( 
        UINT IndexCountPerInstance,
        UINT InstanceCount,
        UINT StartIndexLocation,
        INT BaseVertexLocation,
        UINT StartInstanceLocation);
        
      virtual void STDMETHODCALLTYPE DrawInstanced( 
        UINT VertexCountPerInstance,
        UINT InstanceCount,
        UINT StartVertexLocation,
        UINT StartInstanceLocation);
        
      virtual void STDMETHODCALLTYPE GSSetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE GSSetShader( 
        ID3D11GeometryShader *pShader,
        ID3D11ClassInstance *const *ppClassInstances,
        UINT NumClassInstances);
        
      virtual void STDMETHODCALLTYPE IASetPrimitiveTopology( 
        D3D11_PRIMITIVE_TOPOLOGY Topology);
        
      virtual void STDMETHODCALLTYPE VSSetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE VSSetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState *const *ppSamplers);
        
      virtual void STDMETHODCALLTYPE Begin( 
        ID3D11Asynchronous *pAsync);
        
      virtual void STDMETHODCALLTYPE End( 
        ID3D11Asynchronous *pAsync);
        
      virtual HRESULT STDMETHODCALLTYPE GetData( 
        ID3D11Asynchronous *pAsync,
        void *pData,
        UINT DataSize,
        UINT GetDataFlags);
        
      virtual void STDMETHODCALLTYPE SetPredication( 
        ID3D11Predicate *pPredicate,
        BOOL PredicateValue);
        
      virtual void STDMETHODCALLTYPE GSSetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE GSSetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState *const *ppSamplers);
        
      virtual void STDMETHODCALLTYPE OMSetRenderTargets( 
        UINT NumViews,
        ID3D11RenderTargetView *const *ppRenderTargetViews,
        ID3D11DepthStencilView *pDepthStencilView);
        
      virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews( 
        UINT NumRTVs,
        ID3D11RenderTargetView *const *ppRenderTargetViews,
        ID3D11DepthStencilView *pDepthStencilView,
        UINT UAVStartSlot,
        UINT NumUAVs,
        ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
        const UINT *pUAVInitialCounts);
        
      virtual void STDMETHODCALLTYPE OMSetBlendState( 
        ID3D11BlendState *pBlendState,
        const FLOAT BlendFactor[ 4 ],
        UINT SampleMask);
        
      virtual void STDMETHODCALLTYPE OMSetDepthStencilState( 
        ID3D11DepthStencilState *pDepthStencilState,
        UINT StencilRef);
        
      virtual void STDMETHODCALLTYPE SOSetTargets( 
        UINT NumBuffers,
        ID3D11Buffer *const *ppSOTargets,
        const UINT *pOffsets);
        
      virtual void STDMETHODCALLTYPE DrawAuto();
        
      virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect( 
        ID3D11Buffer *pBufferForArgs,
        UINT AlignedByteOffsetForArgs);
        
      virtual void STDMETHODCALLTYPE DrawInstancedIndirect( 
        ID3D11Buffer *pBufferForArgs,
        UINT AlignedByteOffsetForArgs);
        
      virtual void STDMETHODCALLTYPE Dispatch( 
        UINT ThreadGroupCountX,
        UINT ThreadGroupCountY,
        UINT ThreadGroupCountZ);
        
      virtual void STDMETHODCALLTYPE DispatchIndirect( 
        ID3D11Buffer *pBufferForArgs,
        UINT AlignedByteOffsetForArgs);
        
      virtual void STDMETHODCALLTYPE RSSetState( 
        ID3D11RasterizerState *pRasterizerState);
        
      virtual void STDMETHODCALLTYPE RSSetViewports( 
        UINT NumViewports,
        const D3D11_VIEWPORT *pViewports);
        
      virtual void STDMETHODCALLTYPE RSSetScissorRects( 
        UINT NumRects,
        const D3D11_RECT *pRects);
        
      virtual void STDMETHODCALLTYPE CopySubresourceRegion( 
        ID3D11Resource *pDstResource,
        UINT DstSubresource,
        UINT DstX,
        UINT DstY,
        UINT DstZ,
        ID3D11Resource *pSrcResource,
        UINT SrcSubresource,
        const D3D11_BOX *pSrcBox);
        
      virtual void STDMETHODCALLTYPE CopyResource( 
        ID3D11Resource *pDstResource,
        ID3D11Resource *pSrcResource);
        
      virtual void STDMETHODCALLTYPE UpdateSubresource( 
        ID3D11Resource *pDstResource,
        UINT DstSubresource,
        const D3D11_BOX *pDstBox,
        const void *pSrcData,
        UINT SrcRowPitch,
        UINT SrcDepthPitch);
        
      virtual void STDMETHODCALLTYPE CopyStructureCount( 
        ID3D11Buffer *pDstBuffer,
        UINT DstAlignedByteOffset,
        ID3D11UnorderedAccessView *pSrcView);
        
      virtual void STDMETHODCALLTYPE ClearRenderTargetView( 
        ID3D11RenderTargetView *pRenderTargetView,
        const FLOAT ColorRGBA[ 4 ]);
        
      virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint( 
        ID3D11UnorderedAccessView *pUnorderedAccessView,
        const UINT Values[ 4 ]);
        
      virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat( 
        ID3D11UnorderedAccessView *pUnorderedAccessView,
        const FLOAT Values[ 4 ]);
        
      virtual void STDMETHODCALLTYPE ClearDepthStencilView( 
        ID3D11DepthStencilView *pDepthStencilView,
        UINT ClearFlags,
        FLOAT Depth,
        UINT8 Stencil);
        
      virtual void STDMETHODCALLTYPE GenerateMips( 
        ID3D11ShaderResourceView *pShaderResourceView);
        
      virtual void STDMETHODCALLTYPE SetResourceMinLOD( 
        ID3D11Resource *pResource,
        FLOAT MinLOD);
        
      virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD( 
        ID3D11Resource *pResource);
        
      virtual void STDMETHODCALLTYPE ResolveSubresource( 
        ID3D11Resource *pDstResource,
        UINT DstSubresource,
        ID3D11Resource *pSrcResource,
        UINT SrcSubresource,
        DXGI_FORMAT Format);
        
      virtual void STDMETHODCALLTYPE ExecuteCommandList( 
        ID3D11CommandList *pCommandList,
        BOOL RestoreContextState);
        
      virtual void STDMETHODCALLTYPE HSSetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE HSSetShader( 
        ID3D11HullShader *pHullShader,
        ID3D11ClassInstance *const *ppClassInstances,
        UINT NumClassInstances);
        
      virtual void STDMETHODCALLTYPE HSSetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState *const *ppSamplers);
        
      virtual void STDMETHODCALLTYPE HSSetConstantBuffers( 
         UINT StartSlot,
         UINT NumBuffers,
         ID3D11Buffer *const *ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE DSSetShaderResources( 
         UINT StartSlot,
         UINT NumViews,
         ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE DSSetShader( 
         ID3D11DomainShader *pDomainShader,
         ID3D11ClassInstance *const *ppClassInstances,
         UINT NumClassInstances);
        
      virtual void STDMETHODCALLTYPE DSSetSamplers( 
         UINT StartSlot,
         UINT NumSamplers,
         ID3D11SamplerState *const *ppSamplers);
        
      virtual void STDMETHODCALLTYPE DSSetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE CSSetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView *const *ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews( 
        UINT StartSlot,
        UINT NumUAVs,
        ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
        const UINT *pUAVInitialCounts);
        
      virtual void STDMETHODCALLTYPE CSSetShader( 
        ID3D11ComputeShader *pComputeShader,
        ID3D11ClassInstance *const *ppClassInstances,
        UINT NumClassInstances);
        
      virtual void STDMETHODCALLTYPE CSSetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState *const *ppSamplers);
        
      virtual void STDMETHODCALLTYPE CSSetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer *const *ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE VSGetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE PSGetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView **ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE PSGetShader( 
        ID3D11PixelShader **ppPixelShader,
        ID3D11ClassInstance **ppClassInstances,
        UINT *pNumClassInstances);
        
      virtual void STDMETHODCALLTYPE PSGetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState **ppSamplers);
        
      virtual void STDMETHODCALLTYPE VSGetShader( 
        ID3D11VertexShader **ppVertexShader,
        ID3D11ClassInstance **ppClassInstances,
        UINT *pNumClassInstances);
        
      virtual void STDMETHODCALLTYPE PSGetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE IAGetInputLayout( 
        ID3D11InputLayout **ppInputLayout);
        
      virtual void STDMETHODCALLTYPE IAGetVertexBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppVertexBuffers,
        UINT *pStrides,
        UINT *pOffsets);
        
      virtual void STDMETHODCALLTYPE IAGetIndexBuffer( 
        ID3D11Buffer **pIndexBuffer,
        DXGI_FORMAT *Format,
        UINT *Offset);
        
      virtual void STDMETHODCALLTYPE GSGetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE GSGetShader( 
        ID3D11GeometryShader **ppGeometryShader,
        ID3D11ClassInstance **ppClassInstances,
        UINT *pNumClassInstances);
        
      virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology( 
        D3D11_PRIMITIVE_TOPOLOGY *pTopology);
        
      virtual void STDMETHODCALLTYPE VSGetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView **ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE VSGetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState **ppSamplers);
        
      virtual void STDMETHODCALLTYPE GetPredication( 
        ID3D11Predicate **ppPredicate,
        BOOL *pPredicateValue);
        
      virtual void STDMETHODCALLTYPE GSGetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView **ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE GSGetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState **ppSamplers);
        
      virtual void STDMETHODCALLTYPE OMGetRenderTargets( 
        UINT NumViews,
        ID3D11RenderTargetView **ppRenderTargetViews,
        ID3D11DepthStencilView **ppDepthStencilView);
        
      virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews( 
        UINT NumRTVs,
        ID3D11RenderTargetView **ppRenderTargetViews,
        ID3D11DepthStencilView **ppDepthStencilView,
        UINT UAVStartSlot,
        UINT NumUAVs,
        ID3D11UnorderedAccessView **ppUnorderedAccessViews);
        
      virtual void STDMETHODCALLTYPE OMGetBlendState( 
        ID3D11BlendState **ppBlendState,
        FLOAT BlendFactor[ 4 ],
        UINT *pSampleMask);
        
      virtual void STDMETHODCALLTYPE OMGetDepthStencilState( 
        ID3D11DepthStencilState **ppDepthStencilState,
        UINT *pStencilRef);
        
      virtual void STDMETHODCALLTYPE SOGetTargets( 
        UINT NumBuffers,
        ID3D11Buffer **ppSOTargets);
        
      virtual void STDMETHODCALLTYPE RSGetState( 
        ID3D11RasterizerState **ppRasterizerState);
        
      virtual void STDMETHODCALLTYPE RSGetViewports( 
        UINT *pNumViewports,
        D3D11_VIEWPORT *pViewports);
        
      virtual void STDMETHODCALLTYPE RSGetScissorRects( 
        UINT *pNumRects,
        D3D11_RECT *pRects);
        
      virtual void STDMETHODCALLTYPE HSGetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView **ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE HSGetShader( 
        ID3D11HullShader **ppHullShader,
        ID3D11ClassInstance **ppClassInstances,
        UINT *pNumClassInstances);
        
      virtual void STDMETHODCALLTYPE HSGetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState **ppSamplers);
        
      virtual void STDMETHODCALLTYPE HSGetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE DSGetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView **ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE DSGetShader( 
        ID3D11DomainShader **ppDomainShader,
        ID3D11ClassInstance **ppClassInstances,
        UINT *pNumClassInstances);
        
      virtual void STDMETHODCALLTYPE DSGetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState **ppSamplers);
        
      virtual void STDMETHODCALLTYPE DSGetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE CSGetShaderResources( 
        UINT StartSlot,
        UINT NumViews,
        ID3D11ShaderResourceView **ppShaderResourceViews);
        
      virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews( 
        UINT StartSlot,
        UINT NumUAVs,
        ID3D11UnorderedAccessView **ppUnorderedAccessViews);
        
      virtual void STDMETHODCALLTYPE CSGetShader( 
        ID3D11ComputeShader **ppComputeShader,
        ID3D11ClassInstance **ppClassInstances,
        UINT *pNumClassInstances);
        
      virtual void STDMETHODCALLTYPE CSGetSamplers( 
        UINT StartSlot,
        UINT NumSamplers,
        ID3D11SamplerState **ppSamplers);
        
      virtual void STDMETHODCALLTYPE CSGetConstantBuffers( 
        UINT StartSlot,
        UINT NumBuffers,
        ID3D11Buffer **ppConstantBuffers);
        
      virtual void STDMETHODCALLTYPE ClearState();
        
      virtual void STDMETHODCALLTYPE Flush();
        
      virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType();
        
      virtual UINT STDMETHODCALLTYPE GetContextFlags();
        
      virtual HRESULT STDMETHODCALLTYPE FinishCommandList( 
        BOOL RestoreDeferredContextState,
        ID3D11CommandList **ppCommandList);
      
      // 
      
    private:
      Kernel::Kernel& m_Kernel;
  		ID3D11DeviceContext* m_pDeviceContext;
  		ID3D11Device* m_pDevice;
  		std::shared_ptr<GUI::D3D11Renderer> m_pRenderer;
    };
  }
}
