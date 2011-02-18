/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY
    {
      return m_pDeviceContext->
    } without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

// Hades
#include "DeviceContext.hpp"
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace D3D11
  {
    // 
    
    HRESULT STDMETHODCALLTYPE ID3D11DeviceContextHook::QueryInterface(
      REFIID riid, 
      void** ppvObject)
    {
      return m_pDeviceContext->QueryInterface(riid, ppvObject);
    }
      
    ULONG STDMETHODCALLTYPE ID3D11DeviceContextHook::AddRef()
    {
      return m_pDeviceContext->AddRef();
    }
      
    ULONG STDMETHODCALLTYPE ID3D11DeviceContextHook::Release()
    {
      return m_pDeviceContext->Release();
    }
    
    // 
    
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GetDevice(
      ID3D11Device **ppDevice)
    {
      return m_pDeviceContext->GetDevice(ppDevice);
    }
    
    HRESULT STDMETHODCALLTYPE ID3D11DeviceContextHook::GetPrivateData(
      REFGUID Name,
      UINT *pDataSize,
      void *pData)
    {
      return m_pDeviceContext->GetPrivateData(Name, pDataSize, pData);
    }
    
    HRESULT STDMETHODCALLTYPE ID3D11DeviceContextHook::SetPrivateData(
      REFGUID Name,
      UINT DataSize,
      const void *pData)
    {
      return m_pDeviceContext->SetPrivateData(Name, DataSize, pData);
    }
    
    HRESULT STDMETHODCALLTYPE ID3D11DeviceContextHook::SetPrivateDataInterface(
      REFGUID Name,
      const IUnknown *pUnknown)
    {
      return m_pDeviceContext->SetPrivateDataInterface(Name, pUnknown);
    }
      
    // 
    
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::VSSetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer *const *ppConstantBuffers)
    {
      return m_pDeviceContext->VSSetConstantBuffers(StartSlot, NumBuffers, 
        ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::PSSetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView *const *ppShaderResourceViews)
    {
      return m_pDeviceContext->PSSetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::PSSetShader( 
      ID3D11PixelShader *pPixelShader,
      ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances)
    {
      return m_pDeviceContext->PSSetShader(pPixelShader, ppClassInstances, 
        NumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::PSSetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState *const *ppSamplers)
    {
      return m_pDeviceContext->PSSetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::VSSetShader( 
      ID3D11VertexShader *pVertexShader,
      ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances)
    {
      return m_pDeviceContext->VSSetShader(pVertexShader, ppClassInstances, 
        NumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DrawIndexed( 
      UINT IndexCount,
      UINT StartIndexLocation,
      INT BaseVertexLocation)
    {
      return m_pDeviceContext->DrawIndexed(IndexCount, StartIndexLocation, 
        BaseVertexLocation);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::Draw( 
      UINT VertexCount,
      UINT StartVertexLocation)
    {
      return m_pDeviceContext->Draw(VertexCount, StartVertexLocation);
    }
      
    HRESULT STDMETHODCALLTYPE ID3D11DeviceContextHook::Map( 
      ID3D11Resource *pResource,
      UINT Subresource,
      D3D11_MAP MapType,
      UINT MapFlags,
      D3D11_MAPPED_SUBRESOURCE *pMappedResource)
    {
      return m_pDeviceContext->Map(pResource, Subresource, MapType, MapFlags, 
        pMappedResource);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::Unmap( 
      ID3D11Resource *pResource,
      UINT Subresource)
    {
      return m_pDeviceContext->Unmap(pResource, Subresource);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::PSSetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer *const *ppConstantBuffers)
    {
      return m_pDeviceContext->PSSetConstantBuffers(StartSlot, NumBuffers, 
        ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::IASetInputLayout( 
      ID3D11InputLayout *pInputLayout)
    {
      return m_pDeviceContext->IASetInputLayout(pInputLayout);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::IASetVertexBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer *const *ppVertexBuffers,
      const UINT *pStrides,
      const UINT *pOffsets)
    {
      return m_pDeviceContext->IASetVertexBuffers(StartSlot, NumBuffers, 
        ppVertexBuffers, pStrides, pOffsets);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::IASetIndexBuffer( 
      ID3D11Buffer *pIndexBuffer,
      DXGI_FORMAT Format,
      UINT Offset)
    {
      return m_pDeviceContext->IASetIndexBuffer(pIndexBuffer, Format, Offset);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DrawIndexedInstanced( 
      UINT IndexCountPerInstance,
      UINT InstanceCount,
      UINT StartIndexLocation,
      INT BaseVertexLocation,
      UINT StartInstanceLocation)
    {
      return m_pDeviceContext->DrawIndexedInstanced(IndexCountPerInstance, 
        InstanceCount, StartIndexLocation, BaseVertexLocation, 
        StartInstanceLocation);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DrawInstanced( 
      UINT VertexCountPerInstance,
      UINT InstanceCount,
      UINT StartVertexLocation,
      UINT StartInstanceLocation)
    {
      return m_pDeviceContext->DrawInstanced(VertexCountPerInstance, 
        InstanceCount, StartVertexLocation, StartInstanceLocation);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GSSetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer *const *ppConstantBuffers)
    {
      return m_pDeviceContext->GSSetConstantBuffers(StartSlot, NumBuffers, 
        ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GSSetShader( 
      ID3D11GeometryShader *pShader,
      ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances)
    {
      return m_pDeviceContext->GSSetShader(pShader, ppClassInstances, 
        NumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::IASetPrimitiveTopology( 
      D3D11_PRIMITIVE_TOPOLOGY Topology)
    {
      return m_pDeviceContext->IASetPrimitiveTopology(Topology);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::VSSetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView *const *ppShaderResourceViews)
    {
      return m_pDeviceContext->VSSetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::VSSetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState *const *ppSamplers)
    {
      return m_pDeviceContext->VSSetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::Begin( 
      ID3D11Asynchronous *pAsync)
    {
      return m_pDeviceContext->Begin(pAsync);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::End( 
      ID3D11Asynchronous *pAsync)
    {
      return m_pDeviceContext->End(pAsync);
    }
      
    HRESULT STDMETHODCALLTYPE ID3D11DeviceContextHook::GetData( 
      ID3D11Asynchronous *pAsync,
      void *pData,
      UINT DataSize,
      UINT GetDataFlags)
    {
      return m_pDeviceContext->GetData(pAsync, pData, DataSize, GetDataFlags);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::SetPredication( 
      ID3D11Predicate *pPredicate,
      BOOL PredicateValue)
    {
      return m_pDeviceContext->SetPredication(pPredicate, PredicateValue);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GSSetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView *const *ppShaderResourceViews)
    {
      return m_pDeviceContext->GSSetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GSSetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState *const *ppSamplers)
    {
      return m_pDeviceContext->GSSetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::OMSetRenderTargets( 
      UINT NumViews,
      ID3D11RenderTargetView *const *ppRenderTargetViews,
      ID3D11DepthStencilView *pDepthStencilView)
    {
      return m_pDeviceContext->OMSetRenderTargets(NumViews, 
        ppRenderTargetViews, pDepthStencilView);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::OMSetRenderTargetsAndUnorderedAccessViews( 
      UINT NumRTVs,
      ID3D11RenderTargetView *const *ppRenderTargetViews,
      ID3D11DepthStencilView *pDepthStencilView,
      UINT UAVStartSlot,
      UINT NumUAVs,
      ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
      const UINT *pUAVInitialCounts)
    {
      return m_pDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
        NumRTVs, ppRenderTargetViews, pDepthStencilView, UAVStartSlot, 
        NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::OMSetBlendState( 
      ID3D11BlendState *pBlendState,
      const FLOAT BlendFactor[ 4 ],
      UINT SampleMask)
    {
      return m_pDeviceContext->OMSetBlendState(pBlendState, BlendFactor, 
        SampleMask);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::OMSetDepthStencilState( 
      ID3D11DepthStencilState *pDepthStencilState,
      UINT StencilRef)
    {
      return m_pDeviceContext->OMSetDepthStencilState(pDepthStencilState, 
        StencilRef);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::SOSetTargets( 
      UINT NumBuffers,
      ID3D11Buffer *const *ppSOTargets,
      const UINT *pOffsets)
    {
      return m_pDeviceContext->SOSetTargets(NumBuffers, ppSOTargets, pOffsets);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DrawAuto()
    {
      return m_pDeviceContext->DrawAuto();
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DrawIndexedInstancedIndirect( 
      ID3D11Buffer *pBufferForArgs,
      UINT AlignedByteOffsetForArgs)
    {
      return m_pDeviceContext->DrawIndexedInstancedIndirect(pBufferForArgs, 
        AlignedByteOffsetForArgs);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DrawInstancedIndirect( 
      ID3D11Buffer *pBufferForArgs,
      UINT AlignedByteOffsetForArgs)
    {
      return m_pDeviceContext->DrawInstancedIndirect(pBufferForArgs, 
        AlignedByteOffsetForArgs);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::Dispatch( 
      UINT ThreadGroupCountX,
      UINT ThreadGroupCountY,
      UINT ThreadGroupCountZ)
    {
      return m_pDeviceContext->Dispatch(ThreadGroupCountX, ThreadGroupCountY, 
        ThreadGroupCountZ);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DispatchIndirect( 
      ID3D11Buffer *pBufferForArgs,
      UINT AlignedByteOffsetForArgs)
    {
      return m_pDeviceContext->DispatchIndirect(pBufferForArgs, 
        AlignedByteOffsetForArgs);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::RSSetState( 
      ID3D11RasterizerState *pRasterizerState)
    {
      return m_pDeviceContext->RSSetState(pRasterizerState);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::RSSetViewports( 
      UINT NumViewports,
      const D3D11_VIEWPORT *pViewports)
    {
      return m_pDeviceContext->RSSetViewports(NumViewports, pViewports);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::RSSetScissorRects( 
      UINT NumRects,
      const D3D11_RECT *pRects)
    {
      return m_pDeviceContext->RSSetScissorRects(NumRects, pRects);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CopySubresourceRegion( 
      ID3D11Resource *pDstResource,
      UINT DstSubresource,
      UINT DstX,
      UINT DstY,
      UINT DstZ,
      ID3D11Resource *pSrcResource,
      UINT SrcSubresource,
      const D3D11_BOX *pSrcBox)
    {
      return m_pDeviceContext->CopySubresourceRegion(pDstResource, 
        DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, 
        pSrcBox);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CopyResource( 
      ID3D11Resource *pDstResource,
      ID3D11Resource *pSrcResource)
    {
      return m_pDeviceContext->CopyResource(pDstResource, pSrcResource);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::UpdateSubresource( 
      ID3D11Resource *pDstResource,
      UINT DstSubresource,
      const D3D11_BOX *pDstBox,
      const void *pSrcData,
      UINT SrcRowPitch,
      UINT SrcDepthPitch)
    {
      return m_pDeviceContext->UpdateSubresource(pDstResource, DstSubresource, 
        pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CopyStructureCount( 
      ID3D11Buffer *pDstBuffer,
      UINT DstAlignedByteOffset,
      ID3D11UnorderedAccessView *pSrcView)
    {
      return m_pDeviceContext->CopyStructureCount(pDstBuffer, 
        DstAlignedByteOffset, pSrcView);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::ClearRenderTargetView( 
      ID3D11RenderTargetView *pRenderTargetView,
      const FLOAT ColorRGBA[ 4 ])
    {
      return m_pDeviceContext->ClearRenderTargetView(pRenderTargetView, 
        ColorRGBA);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::ClearUnorderedAccessViewUint( 
      ID3D11UnorderedAccessView *pUnorderedAccessView,
      const UINT Values[ 4 ])
    {
      return m_pDeviceContext->ClearUnorderedAccessViewUint(
        pUnorderedAccessView, Values);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::ClearUnorderedAccessViewFloat( 
      ID3D11UnorderedAccessView *pUnorderedAccessView,
      const FLOAT Values[ 4 ])
    {
      return m_pDeviceContext->ClearUnorderedAccessViewFloat(
        pUnorderedAccessView, Values);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::ClearDepthStencilView( 
      ID3D11DepthStencilView *pDepthStencilView,
      UINT ClearFlags,
      FLOAT Depth,
      UINT8 Stencil)
    {
      return m_pDeviceContext->ClearDepthStencilView(pDepthStencilView, 
        ClearFlags, Depth, Stencil);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GenerateMips( 
      ID3D11ShaderResourceView *pShaderResourceView)
    {
      return m_pDeviceContext->GenerateMips(pShaderResourceView);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::SetResourceMinLOD( 
      ID3D11Resource *pResource,
      FLOAT MinLOD)
    {
      return m_pDeviceContext->SetResourceMinLOD(pResource, MinLOD);
    }
      
    FLOAT STDMETHODCALLTYPE ID3D11DeviceContextHook::GetResourceMinLOD( 
      ID3D11Resource *pResource)
    {
      return m_pDeviceContext->GetResourceMinLOD(pResource);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::ResolveSubresource( 
      ID3D11Resource *pDstResource,
      UINT DstSubresource,
      ID3D11Resource *pSrcResource,
      UINT SrcSubresource,
      DXGI_FORMAT Format)
    {
      return m_pDeviceContext->ResolveSubresource(pDstResource, 
        DstSubresource, pSrcResource, SrcSubresource, Format);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::ExecuteCommandList( 
      ID3D11CommandList *pCommandList,
      BOOL RestoreContextState)
    {
      return m_pDeviceContext->ExecuteCommandList(pCommandList, 
        RestoreContextState);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::HSSetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView *const *ppShaderResourceViews)
    {
      return m_pDeviceContext->HSSetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::HSSetShader( 
      ID3D11HullShader *pHullShader,
      ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances)
    {
      return m_pDeviceContext->HSSetShader(pHullShader, ppClassInstances, 
        NumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::HSSetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState *const *ppSamplers)
    {
      return m_pDeviceContext->HSSetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::HSSetConstantBuffers( 
       UINT StartSlot,
       UINT NumBuffers,
       ID3D11Buffer *const *ppConstantBuffers)
    {
      return m_pDeviceContext->HSSetConstantBuffers(StartSlot, NumBuffers, 
        ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DSSetShaderResources( 
       UINT StartSlot,
       UINT NumViews,
       ID3D11ShaderResourceView *const *ppShaderResourceViews)
    {
      return m_pDeviceContext->DSSetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DSSetShader( 
       ID3D11DomainShader *pDomainShader,
       ID3D11ClassInstance *const *ppClassInstances,
       UINT NumClassInstances)
    {
      return m_pDeviceContext->DSSetShader(pDomainShader, ppClassInstances, 
        NumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DSSetSamplers( 
       UINT StartSlot,
       UINT NumSamplers,
       ID3D11SamplerState *const *ppSamplers)
    {
      return m_pDeviceContext->DSSetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DSSetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer *const *ppConstantBuffers)
    {
      return m_pDeviceContext->DSSetConstantBuffers(StartSlot, NumBuffers, 
        ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CSSetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView *const *ppShaderResourceViews)
    {
      return m_pDeviceContext->CSSetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CSSetUnorderedAccessViews( 
      UINT StartSlot,
      UINT NumUAVs,
      ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
      const UINT *pUAVInitialCounts)
    {
      return m_pDeviceContext->CSSetUnorderedAccessViews(StartSlot, NumUAVs, 
        ppUnorderedAccessViews, pUAVInitialCounts);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CSSetShader( 
      ID3D11ComputeShader *pComputeShader,
      ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances)
    {
      return m_pDeviceContext->CSSetShader(pComputeShader, ppClassInstances, 
        NumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CSSetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState *const *ppSamplers)
    {
      return m_pDeviceContext->CSSetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CSSetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer *const *ppConstantBuffers)
    {
      return m_pDeviceContext->CSSetConstantBuffers(StartSlot, NumBuffers, 
        ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::VSGetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer **ppConstantBuffers)
    {
      return m_pDeviceContext->VSGetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::PSGetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView **ppShaderResourceViews)
    {
      return m_pDeviceContext->PSGetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::PSGetShader( 
      ID3D11PixelShader **ppPixelShader,
      ID3D11ClassInstance **ppClassInstances,
      UINT *pNumClassInstances)
    {
      return m_pDeviceContext->PSGetShader(ppPixelShader, ppClassInstances, 
        pNumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::PSGetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState **ppSamplers)
    {
      return m_pDeviceContext->PSGetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::VSGetShader( 
      ID3D11VertexShader **ppVertexShader,
      ID3D11ClassInstance **ppClassInstances,
      UINT *pNumClassInstances)
    {
      return m_pDeviceContext->VSGetShader(ppVertexShader, 
        ppClassInstances, pNumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::PSGetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer **ppConstantBuffers)
    {
      return m_pDeviceContext->PSGetConstantBuffers(StartSlot, 
        NumBuffers, ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::IAGetInputLayout( 
      ID3D11InputLayout **ppInputLayout)
    {
      return m_pDeviceContext->IAGetInputLayout(ppInputLayout);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::IAGetVertexBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer **ppVertexBuffers,
      UINT *pStrides,
      UINT *pOffsets)
    {
      return m_pDeviceContext->IAGetVertexBuffers(StartSlot, NumBuffers, 
        ppVertexBuffers, pStrides, pOffsets);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::IAGetIndexBuffer( 
      ID3D11Buffer **pIndexBuffer,
      DXGI_FORMAT *Format,
      UINT *Offset)
    {
      return m_pDeviceContext->IAGetIndexBuffer(pIndexBuffer, Format, 
        Offset);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GSGetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer **ppConstantBuffers)
    {
      return m_pDeviceContext->GSGetConstantBuffers(StartSlot, NumBuffers, 
        ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GSGetShader( 
      ID3D11GeometryShader **ppGeometryShader,
      ID3D11ClassInstance **ppClassInstances,
      UINT *pNumClassInstances)
    {
      return m_pDeviceContext->GSGetShader(ppGeometryShader, ppClassInstances, 
        pNumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::IAGetPrimitiveTopology( 
      D3D11_PRIMITIVE_TOPOLOGY *pTopology)
    {
      return m_pDeviceContext->IAGetPrimitiveTopology(pTopology);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::VSGetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView **ppShaderResourceViews)
    {
      return m_pDeviceContext->VSGetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::VSGetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState **ppSamplers)
    {
      return m_pDeviceContext->VSGetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GetPredication( 
      ID3D11Predicate **ppPredicate,
      BOOL *pPredicateValue)
    {
      return m_pDeviceContext->GetPredication(ppPredicate, pPredicateValue);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GSGetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView **ppShaderResourceViews)
    {
      return m_pDeviceContext->GSGetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::GSGetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState **ppSamplers)
    {
      return m_pDeviceContext->GSGetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::OMGetRenderTargets( 
      UINT NumViews,
      ID3D11RenderTargetView **ppRenderTargetViews,
      ID3D11DepthStencilView **ppDepthStencilView)
    {
      return m_pDeviceContext->OMGetRenderTargets(NumViews, 
        ppRenderTargetViews, ppDepthStencilView);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::OMGetRenderTargetsAndUnorderedAccessViews( 
      UINT NumRTVs,
      ID3D11RenderTargetView **ppRenderTargetViews,
      ID3D11DepthStencilView **ppDepthStencilView,
      UINT UAVStartSlot,
      UINT NumUAVs,
      ID3D11UnorderedAccessView **ppUnorderedAccessViews)
    {
      return m_pDeviceContext->OMGetRenderTargetsAndUnorderedAccessViews(
        NumRTVs, ppRenderTargetViews, ppDepthStencilView, UAVStartSlot, 
        NumUAVs, ppUnorderedAccessViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::OMGetBlendState( 
      ID3D11BlendState **ppBlendState,
      FLOAT BlendFactor[ 4 ],
      UINT *pSampleMask)
    {
      return m_pDeviceContext->OMGetBlendState(ppBlendState, BlendFactor, 
        pSampleMask);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::OMGetDepthStencilState( 
      ID3D11DepthStencilState **ppDepthStencilState,
      UINT *pStencilRef)
    {
      return m_pDeviceContext->OMGetDepthStencilState(ppDepthStencilState, 
        pStencilRef);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::SOGetTargets( 
      UINT NumBuffers,
      ID3D11Buffer **ppSOTargets)
    {
      return m_pDeviceContext->SOGetTargets(NumBuffers, ppSOTargets);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::RSGetState( 
      ID3D11RasterizerState **ppRasterizerState)
    {
      return m_pDeviceContext->RSGetState(ppRasterizerState);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::RSGetViewports( 
      UINT *pNumViewports,
      D3D11_VIEWPORT *pViewports)
    {
      return m_pDeviceContext->RSGetViewports(pNumViewports, pViewports);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::RSGetScissorRects( 
      UINT *pNumRects,
      D3D11_RECT *pRects)
    {
      return m_pDeviceContext->RSGetScissorRects(pNumRects, pRects);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::HSGetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView **ppShaderResourceViews)
    {
      return m_pDeviceContext->HSGetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::HSGetShader( 
      ID3D11HullShader **ppHullShader,
      ID3D11ClassInstance **ppClassInstances,
      UINT *pNumClassInstances)
    {
      return m_pDeviceContext->HSGetShader(ppHullShader, ppClassInstances, 
        pNumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::HSGetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState **ppSamplers)
    {
      return m_pDeviceContext->HSGetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::HSGetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer **ppConstantBuffers)
    {
      return m_pDeviceContext->HSGetConstantBuffers(StartSlot, NumBuffers, 
        ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DSGetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView **ppShaderResourceViews)
    {
      return m_pDeviceContext->DSGetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DSGetShader( 
      ID3D11DomainShader **ppDomainShader,
      ID3D11ClassInstance **ppClassInstances,
      UINT *pNumClassInstances)
    {
      return m_pDeviceContext->DSGetShader(ppDomainShader, ppClassInstances, 
        pNumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DSGetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState **ppSamplers)
    {
      return m_pDeviceContext->DSGetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::DSGetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer **ppConstantBuffers)
    {
      return m_pDeviceContext->DSGetConstantBuffers(StartSlot, NumBuffers, 
        ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CSGetShaderResources( 
      UINT StartSlot,
      UINT NumViews,
      ID3D11ShaderResourceView **ppShaderResourceViews)
    {
      return m_pDeviceContext->CSGetShaderResources(StartSlot, NumViews, 
        ppShaderResourceViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CSGetUnorderedAccessViews( 
      UINT StartSlot,
      UINT NumUAVs,
      ID3D11UnorderedAccessView **ppUnorderedAccessViews)
    {
      return m_pDeviceContext->CSGetUnorderedAccessViews(StartSlot, NumUAVs, 
        ppUnorderedAccessViews);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CSGetShader( 
      ID3D11ComputeShader **ppComputeShader,
      ID3D11ClassInstance **ppClassInstances,
      UINT *pNumClassInstances)
    {
      return m_pDeviceContext->CSGetShader(ppComputeShader, ppClassInstances, 
        pNumClassInstances);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CSGetSamplers( 
      UINT StartSlot,
      UINT NumSamplers,
      ID3D11SamplerState **ppSamplers)
    {
      return m_pDeviceContext->CSGetSamplers(StartSlot, NumSamplers, 
        ppSamplers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::CSGetConstantBuffers( 
      UINT StartSlot,
      UINT NumBuffers,
      ID3D11Buffer **ppConstantBuffers)
    {
      return m_pDeviceContext->CSGetConstantBuffers(StartSlot, NumBuffers, 
        ppConstantBuffers);
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::ClearState()
    {
      return m_pDeviceContext->ClearState();
    }
      
    void STDMETHODCALLTYPE ID3D11DeviceContextHook::Flush()
    {
      return m_pDeviceContext->Flush();
    }
      
    D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE ID3D11DeviceContextHook::GetType()
    {
      return m_pDeviceContext->GetType();
    }
      
    UINT STDMETHODCALLTYPE ID3D11DeviceContextHook::GetContextFlags()
    {
      return m_pDeviceContext->GetContextFlags();
    }
      
    HRESULT STDMETHODCALLTYPE ID3D11DeviceContextHook::FinishCommandList( 
      BOOL RestoreDeferredContextState,
      ID3D11CommandList **ppCommandList)
    {
      return m_pDeviceContext->FinishCommandList(RestoreDeferredContextState, 
        ppCommandList);
    }
    
    // 
  }
}

