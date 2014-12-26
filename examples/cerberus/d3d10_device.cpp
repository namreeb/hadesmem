// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d10_device.hpp"

#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>

#include "d3d10.hpp"

namespace hadesmem
{
namespace cerberus
{
HRESULT WINAPI D3D10DeviceProxy::QueryInterface(REFIID riid, void** obj)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  last_error_preserver.Revert();
  auto const ret = device_->QueryInterface(riid, obj);
  last_error_preserver.Update();

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (*obj == device_)
    {
      refs_++;
      *obj = this;
    }
    else if (riid == __uuidof(ID3D10Device1))
    {
      *obj = new D3D10DeviceProxy(static_cast<ID3D10Device1*>(*obj));
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("WARNING! Unhandled interface.");
      HADESMEM_DETAIL_ASSERT(false);
      static_cast<IUnknown*>(*obj)->Release();
      return E_NOINTERFACE;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

ULONG WINAPI D3D10DeviceProxy::AddRef()
{
  refs_++;
  auto const ret = device_->AddRef();
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);
  return ret;
}

ULONG WINAPI D3D10DeviceProxy::Release()
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  refs_--;
  HADESMEM_DETAIL_ASSERT(refs_ >= 0);

  if (refs_ == 0)
  {
    Cleanup();
  }

  last_error_preserver.Revert();
  auto const ret = device_->Release();
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);

  if (ret == 0)
  {
    delete this;
  }

  return ret;
}

// ID3D10Device
void WINAPI D3D10DeviceProxy::VSSetConstantBuffers(
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
    UINT NumBuffers,
  _In_reads_opt_(NumBuffers) ID3D10Buffer* const* ppConstantBuffers)
{
  return device_->VSSetConstantBuffers(
    StartSlot, NumBuffers, ppConstantBuffers);
}

void WINAPI D3D10DeviceProxy::PSSetShaderResources(
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
    UINT NumViews,
  _In_reads_opt_(NumViews)
    ID3D10ShaderResourceView* const* ppShaderResourceViews)
{
  return device_->PSSetShaderResources(
    StartSlot, NumViews, ppShaderResourceViews);
}

void WINAPI
  D3D10DeviceProxy::PSSetShader(_In_opt_ ID3D10PixelShader* pPixelShader)
{
  return device_->PSSetShader(pPixelShader);
}

void WINAPI D3D10DeviceProxy::PSSetSamplers(
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
    UINT NumSamplers,
  _In_reads_opt_(NumSamplers) ID3D10SamplerState* const* ppSamplers)
{
  return device_->PSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI
  D3D10DeviceProxy::VSSetShader(_In_opt_ ID3D10VertexShader* pVertexShader)
{
  return device_->VSSetShader(pVertexShader);
}

void WINAPI D3D10DeviceProxy::DrawIndexed(_In_ UINT IndexCount,
                                          _In_ UINT StartIndexLocation,
                                          _In_ INT BaseVertexLocation)
{
  return device_->DrawIndexed(
    IndexCount, StartIndexLocation, BaseVertexLocation);
}

void WINAPI
  D3D10DeviceProxy::Draw(_In_ UINT VertexCount, _In_ UINT StartVertexLocation)
{
  return device_->Draw(VertexCount, StartVertexLocation);
}

void WINAPI D3D10DeviceProxy::PSSetConstantBuffers(
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
    UINT NumBuffers,
  _In_reads_opt_(NumBuffers) ID3D10Buffer* const* ppConstantBuffers)
{
  return device_->PSSetConstantBuffers(
    StartSlot, NumBuffers, ppConstantBuffers);
}

void WINAPI
  D3D10DeviceProxy::IASetInputLayout(_In_opt_ ID3D10InputLayout* pInputLayout)
{
  return device_->IASetInputLayout(pInputLayout);
}

void WINAPI D3D10DeviceProxy::IASetVertexBuffers(
  _In_range_(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot,
  _In_range_(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
    UINT NumBuffers,
  _In_reads_opt_(NumBuffers) ID3D10Buffer* const* ppVertexBuffers,
  _In_reads_opt_(NumBuffers) const UINT* pStrides,
  _In_reads_opt_(NumBuffers) const UINT* pOffsets)
{
  return device_->IASetVertexBuffers(
    StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}

void WINAPI
  D3D10DeviceProxy::IASetIndexBuffer(_In_opt_ ID3D10Buffer* pIndexBuffer,
                                     _In_ DXGI_FORMAT Format,
                                     _In_ UINT Offset)
{
  return device_->IASetIndexBuffer(pIndexBuffer, Format, Offset);
}

void WINAPI
  D3D10DeviceProxy::DrawIndexedInstanced(_In_ UINT IndexCountPerInstance,
                                         _In_ UINT InstanceCount,
                                         _In_ UINT StartIndexLocation,
                                         _In_ INT BaseVertexLocation,
                                         _In_ UINT StartInstanceLocation)
{
  return device_->DrawIndexedInstanced(IndexCountPerInstance,
                                       InstanceCount,
                                       StartIndexLocation,
                                       BaseVertexLocation,
                                       StartInstanceLocation);
}

void WINAPI D3D10DeviceProxy::DrawInstanced(_In_ UINT VertexCountPerInstance,
                                            _In_ UINT InstanceCount,
                                            _In_ UINT StartVertexLocation,
                                            _In_ UINT StartInstanceLocation)
{
  return device_->DrawInstanced(VertexCountPerInstance,
                                InstanceCount,
                                StartVertexLocation,
                                StartInstanceLocation);
}

void WINAPI D3D10DeviceProxy::GSSetConstantBuffers(
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
    UINT NumBuffers,
  _In_reads_opt_(NumBuffers) ID3D10Buffer* const* ppConstantBuffers)
{
  return device_->GSSetConstantBuffers(
    StartSlot, NumBuffers, ppConstantBuffers);
}

void WINAPI
  D3D10DeviceProxy::GSSetShader(_In_opt_ ID3D10GeometryShader* pShader)
{
  return device_->GSSetShader(pShader);
}

void WINAPI
  D3D10DeviceProxy::IASetPrimitiveTopology(_In_ D3D10_PRIMITIVE_TOPOLOGY
                                             Topology)
{
  return device_->IASetPrimitiveTopology(Topology);
}

void WINAPI D3D10DeviceProxy::VSSetShaderResources(
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
    UINT NumViews,
  _In_reads_opt_(NumViews)
    ID3D10ShaderResourceView* const* ppShaderResourceViews)
{
  return device_->VSSetShaderResources(
    StartSlot, NumViews, ppShaderResourceViews);
}

void WINAPI D3D10DeviceProxy::VSSetSamplers(
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
    UINT NumSamplers,
  _In_reads_opt_(NumSamplers) ID3D10SamplerState* const* ppSamplers)
{
  return device_->VSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI
  D3D10DeviceProxy::SetPredication(_In_opt_ ID3D10Predicate* pPredicate,
                                   _In_ BOOL PredicateValue)
{
  return device_->SetPredication(pPredicate, PredicateValue);
}

void WINAPI D3D10DeviceProxy::GSSetShaderResources(
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
    UINT NumViews,
  _In_reads_opt_(NumViews)
    ID3D10ShaderResourceView* const* ppShaderResourceViews)
{
  return device_->GSSetShaderResources(
    StartSlot, NumViews, ppShaderResourceViews);
}

void WINAPI D3D10DeviceProxy::GSSetSamplers(
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
    UINT NumSamplers,
  _In_reads_opt_(NumSamplers) ID3D10SamplerState* const* ppSamplers)
{
  return device_->GSSetSamplers(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI D3D10DeviceProxy::OMSetRenderTargets(
  _In_range_(0, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumViews,
  _In_reads_opt_(NumViews) ID3D10RenderTargetView* const* ppRenderTargetViews,
  _In_opt_ ID3D10DepthStencilView* pDepthStencilView)
{
  return device_->OMSetRenderTargets(
    NumViews, ppRenderTargetViews, pDepthStencilView);
}

void WINAPI
  D3D10DeviceProxy::OMSetBlendState(_In_opt_ ID3D10BlendState* pBlendState,
                                    _In_ const FLOAT BlendFactor[4],
                                    _In_ UINT SampleMask)
{
  return device_->OMSetBlendState(pBlendState, BlendFactor, SampleMask);
}

void WINAPI D3D10DeviceProxy::OMSetDepthStencilState(
  _In_opt_ ID3D10DepthStencilState* pDepthStencilState, _In_ UINT StencilRef)
{
  return device_->OMSetDepthStencilState(pDepthStencilState, StencilRef);
}

void WINAPI D3D10DeviceProxy::SOSetTargets(
  _In_range_(0, D3D10_SO_BUFFER_SLOT_COUNT) UINT NumBuffers,
  _In_reads_opt_(NumBuffers) ID3D10Buffer* const* ppSOTargets,
  _In_reads_opt_(NumBuffers) const UINT* pOffsets)
{
  return device_->SOSetTargets(NumBuffers, ppSOTargets, pOffsets);
}

void WINAPI D3D10DeviceProxy::DrawAuto()
{
  return device_->DrawAuto();
}

void WINAPI
  D3D10DeviceProxy::RSSetState(_In_opt_ ID3D10RasterizerState* pRasterizerState)
{
  return device_->RSSetState(pRasterizerState);
}

void WINAPI D3D10DeviceProxy::RSSetViewports(
  _In_range_(0, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)
    UINT NumViewports,
  _In_reads_opt_(NumViewports) const D3D10_VIEWPORT* pViewports)
{
  return device_->RSSetViewports(NumViewports, pViewports);
}

void WINAPI D3D10DeviceProxy::RSSetScissorRects(
  _In_range_(0, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)
    UINT NumRects,
  _In_reads_opt_(NumRects) const D3D10_RECT* pRects)
{
  return device_->RSSetScissorRects(NumRects, pRects);
}

void WINAPI
  D3D10DeviceProxy::CopySubresourceRegion(_In_ ID3D10Resource* pDstResource,
                                          _In_ UINT DstSubresource,
                                          _In_ UINT DstX,
                                          _In_ UINT DstY,
                                          _In_ UINT DstZ,
                                          _In_ ID3D10Resource* pSrcResource,
                                          _In_ UINT SrcSubresource,
                                          _In_opt_ const D3D10_BOX* pSrcBox)
{
  return device_->CopySubresourceRegion(pDstResource,
                                        DstSubresource,
                                        DstX,
                                        DstY,
                                        DstZ,
                                        pSrcResource,
                                        SrcSubresource,
                                        pSrcBox);
}

void WINAPI D3D10DeviceProxy::CopyResource(_In_ ID3D10Resource* pDstResource,
                                           _In_ ID3D10Resource* pSrcResource)
{
  return device_->CopyResource(pDstResource, pSrcResource);
}

void WINAPI
  D3D10DeviceProxy::UpdateSubresource(_In_ ID3D10Resource* pDstResource,
                                      _In_ UINT DstSubresource,
                                      _In_opt_ const D3D10_BOX* pDstBox,
                                      _In_ const void* pSrcData,
                                      _In_ UINT SrcRowPitch,
                                      _In_ UINT SrcDepthPitch)
{
  return device_->UpdateSubresource(pDstResource,
                                    DstSubresource,
                                    pDstBox,
                                    pSrcData,
                                    SrcRowPitch,
                                    SrcDepthPitch);
}

void WINAPI D3D10DeviceProxy::ClearRenderTargetView(
  _In_ ID3D10RenderTargetView* pRenderTargetView, _In_ const FLOAT ColorRGBA[4])
{
  return device_->ClearRenderTargetView(pRenderTargetView, ColorRGBA);
}

void WINAPI D3D10DeviceProxy::ClearDepthStencilView(
  _In_ ID3D10DepthStencilView* pDepthStencilView,
  _In_ UINT ClearFlags,
  _In_ FLOAT Depth,
  _In_ UINT8 Stencil)
{
  return device_->ClearDepthStencilView(
    pDepthStencilView, ClearFlags, Depth, Stencil);
}

void WINAPI D3D10DeviceProxy::GenerateMips(
  _In_ ID3D10ShaderResourceView* pShaderResourceView)
{
  return device_->GenerateMips(pShaderResourceView);
}

void WINAPI
  D3D10DeviceProxy::ResolveSubresource(_In_ ID3D10Resource* pDstResource,
                                       _In_ UINT DstSubresource,
                                       _In_ ID3D10Resource* pSrcResource,
                                       _In_ UINT SrcSubresource,
                                       _In_ DXGI_FORMAT Format)
{
  return device_->ResolveSubresource(
    pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
}

void WINAPI D3D10DeviceProxy::VSGetConstantBuffers(
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
    UINT NumBuffers,
  _Out_writes_opt_(NumBuffers) ID3D10Buffer** ppConstantBuffers)
{
  return device_->VSGetConstantBuffers(
    StartSlot, NumBuffers, ppConstantBuffers);
}

void WINAPI D3D10DeviceProxy::PSGetShaderResources(
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
    UINT NumViews,
  _Out_writes_opt_(NumViews) ID3D10ShaderResourceView** ppShaderResourceViews)
{
  return device_->PSGetShaderResources(
    StartSlot, NumViews, ppShaderResourceViews);
}

void WINAPI
  D3D10DeviceProxy::PSGetShader(_Out_ ID3D10PixelShader** ppPixelShader)
{
  return device_->PSGetShader(ppPixelShader);
}

void WINAPI D3D10DeviceProxy::PSGetSamplers(
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
    UINT NumSamplers,
  _Out_writes_opt_(NumSamplers) ID3D10SamplerState** ppSamplers)
{
  return device_->PSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI
  D3D10DeviceProxy::VSGetShader(_Out_ ID3D10VertexShader** ppVertexShader)
{
  return device_->VSGetShader(ppVertexShader);
}

void WINAPI D3D10DeviceProxy::PSGetConstantBuffers(
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
    UINT NumBuffers,
  _Out_writes_opt_(NumBuffers) ID3D10Buffer** ppConstantBuffers)
{
  return device_->PSGetConstantBuffers(
    StartSlot, NumBuffers, ppConstantBuffers);
}

void WINAPI
  D3D10DeviceProxy::IAGetInputLayout(_Out_ ID3D10InputLayout** ppInputLayout)
{
  return device_->IAGetInputLayout(ppInputLayout);
}

void WINAPI D3D10DeviceProxy::IAGetVertexBuffers(
  _In_range_(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot,
  _In_range_(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
    UINT NumBuffers,
  _Out_writes_opt_(NumBuffers) ID3D10Buffer** ppVertexBuffers,
  _Out_writes_opt_(NumBuffers) UINT* pStrides,
  _Out_writes_opt_(NumBuffers) UINT* pOffsets)
{
  return device_->IAGetVertexBuffers(
    StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
}

void WINAPI
  D3D10DeviceProxy::IAGetIndexBuffer(_Out_opt_ ID3D10Buffer** pIndexBuffer,
                                     _Out_opt_ DXGI_FORMAT* Format,
                                     _Out_opt_ UINT* Offset)
{
  return device_->IAGetIndexBuffer(pIndexBuffer, Format, Offset);
}

void WINAPI D3D10DeviceProxy::GSGetConstantBuffers(
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
    UINT NumBuffers,
  _Out_writes_opt_(NumBuffers) ID3D10Buffer** ppConstantBuffers)
{
  return device_->GSGetConstantBuffers(
    StartSlot, NumBuffers, ppConstantBuffers);
}

void WINAPI
  D3D10DeviceProxy::GSGetShader(_Out_ ID3D10GeometryShader** ppGeometryShader)
{
  return device_->GSGetShader(ppGeometryShader);
}

void WINAPI D3D10DeviceProxy::IAGetPrimitiveTopology(
  _Out_ D3D10_PRIMITIVE_TOPOLOGY* pTopology)
{
  return device_->IAGetPrimitiveTopology(pTopology);
}

void WINAPI D3D10DeviceProxy::VSGetShaderResources(
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
    UINT NumViews,
  _Out_writes_opt_(NumViews) ID3D10ShaderResourceView** ppShaderResourceViews)
{
  return device_->VSGetShaderResources(
    StartSlot, NumViews, ppShaderResourceViews);
}

void WINAPI D3D10DeviceProxy::VSGetSamplers(
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
    UINT NumSamplers,
  _Out_writes_opt_(NumSamplers) ID3D10SamplerState** ppSamplers)
{
  return device_->VSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI
  D3D10DeviceProxy::GetPredication(_Out_opt_ ID3D10Predicate** ppPredicate,
                                   _Out_opt_ BOOL* pPredicateValue)
{
  return device_->GetPredication(ppPredicate, pPredicateValue);
}

void WINAPI D3D10DeviceProxy::GSGetShaderResources(
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
    UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
    UINT NumViews,
  _Out_writes_opt_(NumViews) ID3D10ShaderResourceView** ppShaderResourceViews)
{
  return device_->GSGetShaderResources(
    StartSlot, NumViews, ppShaderResourceViews);
}

void WINAPI D3D10DeviceProxy::GSGetSamplers(
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
  _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
    UINT NumSamplers,
  _Out_writes_opt_(NumSamplers) ID3D10SamplerState** ppSamplers)
{
  return device_->GSGetSamplers(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI D3D10DeviceProxy::OMGetRenderTargets(
  _In_range_(0, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumViews,
  _Out_writes_opt_(NumViews) ID3D10RenderTargetView** ppRenderTargetViews,
  _Out_opt_ ID3D10DepthStencilView** ppDepthStencilView)
{
  return device_->OMGetRenderTargets(
    NumViews, ppRenderTargetViews, ppDepthStencilView);
}

void WINAPI
  D3D10DeviceProxy::OMGetBlendState(_Out_opt_ ID3D10BlendState** ppBlendState,
                                    _Out_opt_ FLOAT BlendFactor[4],
                                    _Out_opt_ UINT* pSampleMask)
{
  return device_->OMGetBlendState(ppBlendState, BlendFactor, pSampleMask);
}

void WINAPI D3D10DeviceProxy::OMGetDepthStencilState(
  _Out_opt_ ID3D10DepthStencilState** ppDepthStencilState,
  _Out_opt_ UINT* pStencilRef)
{
  return device_->OMGetDepthStencilState(ppDepthStencilState, pStencilRef);
}

void WINAPI
  D3D10DeviceProxy::SOGetTargets(_In_range_(0, D3D10_SO_BUFFER_SLOT_COUNT)
                                   UINT NumBuffers,
                                 _Out_writes_opt_(NumBuffers)
                                   ID3D10Buffer** ppSOTargets,
                                 _Out_writes_opt_(NumBuffers) UINT* pOffsets)
{
  return device_->SOGetTargets(NumBuffers, ppSOTargets, pOffsets);
}

void WINAPI
  D3D10DeviceProxy::RSGetState(_Out_ ID3D10RasterizerState** ppRasterizerState)
{
  return device_->RSGetState(ppRasterizerState);
}

void WINAPI D3D10DeviceProxy::RSGetViewports(
  _Inout_ /*_range(0, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/ UINT*
    NumViewports,
  _Out_writes_opt_(*NumViewports) D3D10_VIEWPORT* pViewports)
{
  return device_->RSGetViewports(NumViewports, pViewports);
}

void WINAPI D3D10DeviceProxy::RSGetScissorRects(
  _Inout_ /*_range(0, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/ UINT*
    NumRects,
  _Out_writes_opt_(*NumRects) D3D10_RECT* pRects)
{
  return device_->RSGetScissorRects(NumRects, pRects);
}

HRESULT WINAPI D3D10DeviceProxy::GetDeviceRemovedReason()
{
  return device_->GetDeviceRemovedReason();
}

HRESULT WINAPI D3D10DeviceProxy::SetExceptionMode(UINT RaiseFlags)
{
  return device_->SetExceptionMode(RaiseFlags);
}

UINT WINAPI D3D10DeviceProxy::GetExceptionMode()
{
  return device_->GetExceptionMode();
}

HRESULT WINAPI D3D10DeviceProxy::GetPrivateData(
  _In_ REFGUID guid,
  _Inout_ UINT* pDataSize,
  _Out_writes_bytes_opt_(*pDataSize) void* pData)
{
  return device_->GetPrivateData(guid, pDataSize, pData);
}

HRESULT WINAPI D3D10DeviceProxy::SetPrivateData(_In_ REFGUID guid,
                                                _In_ UINT DataSize,
                                                _In_reads_bytes_opt_(DataSize)
                                                  const void* pData)
{
  return device_->SetPrivateData(guid, DataSize, pData);
}

HRESULT WINAPI
  D3D10DeviceProxy::SetPrivateDataInterface(_In_ REFGUID guid,
                                            _In_opt_ const IUnknown* pData)
{
  return device_->SetPrivateDataInterface(guid, pData);
}

void WINAPI D3D10DeviceProxy::ClearState()
{
  return device_->ClearState();
}

void WINAPI D3D10DeviceProxy::Flush()
{
  return device_->Flush();
}

HRESULT WINAPI D3D10DeviceProxy::CreateBuffer(
  _In_ const D3D10_BUFFER_DESC* pDesc,
  _In_opt_ const D3D10_SUBRESOURCE_DATA* pInitialData,
  _Out_opt_ ID3D10Buffer** ppBuffer)
{
  return device_->CreateBuffer(pDesc, pInitialData, ppBuffer);
}

HRESULT WINAPI D3D10DeviceProxy::CreateTexture1D(
  _In_ const D3D10_TEXTURE1D_DESC* pDesc,
  _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))
    const D3D10_SUBRESOURCE_DATA* pInitialData,
  _Out_ ID3D10Texture1D** ppTexture1D)
{
  return device_->CreateTexture1D(pDesc, pInitialData, ppTexture1D);
}

HRESULT WINAPI D3D10DeviceProxy::CreateTexture2D(
  _In_ const D3D10_TEXTURE2D_DESC* pDesc,
  _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))
    const D3D10_SUBRESOURCE_DATA* pInitialData,
  _Out_ ID3D10Texture2D** ppTexture2D)
{
  return device_->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
}

HRESULT WINAPI D3D10DeviceProxy::CreateTexture3D(
  _In_ const D3D10_TEXTURE3D_DESC* pDesc,
  _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))
    const D3D10_SUBRESOURCE_DATA* pInitialData,
  _Out_ ID3D10Texture3D** ppTexture3D)
{
  return device_->CreateTexture3D(pDesc, pInitialData, ppTexture3D);
}

HRESULT WINAPI D3D10DeviceProxy::CreateShaderResourceView(
  _In_ ID3D10Resource* pResource,
  _In_opt_ const D3D10_SHADER_RESOURCE_VIEW_DESC* pDesc,
  _Out_opt_ ID3D10ShaderResourceView** ppSRView)
{
  return device_->CreateShaderResourceView(pResource, pDesc, ppSRView);
}

HRESULT WINAPI D3D10DeviceProxy::CreateRenderTargetView(
  _In_ ID3D10Resource* pResource,
  _In_opt_ const D3D10_RENDER_TARGET_VIEW_DESC* pDesc,
  _Out_opt_ ID3D10RenderTargetView** ppRTView)
{
  return device_->CreateRenderTargetView(pResource, pDesc, ppRTView);
}

HRESULT WINAPI D3D10DeviceProxy::CreateDepthStencilView(
  _In_ ID3D10Resource* pResource,
  _In_opt_ const D3D10_DEPTH_STENCIL_VIEW_DESC* pDesc,
  _Out_opt_ ID3D10DepthStencilView** ppDepthStencilView)
{
  return device_->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView);
}

HRESULT WINAPI D3D10DeviceProxy::CreateInputLayout(
  _In_reads_(NumElements) const D3D10_INPUT_ELEMENT_DESC* pInputElementDescs,
  _In_range_(0, D3D10_1_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)
    UINT NumElements,
  _In_reads_(BytecodeLength) const void* pShaderBytecodeWithInputSignature,
  _In_ SIZE_T BytecodeLength,
  _Out_opt_ ID3D10InputLayout** ppInputLayout)
{
  return device_->CreateInputLayout(pInputElementDescs,
                                    NumElements,
                                    pShaderBytecodeWithInputSignature,
                                    BytecodeLength,
                                    ppInputLayout);
}

HRESULT WINAPI D3D10DeviceProxy::CreateVertexShader(
  _In_reads_(BytecodeLength) const void* pShaderBytecode,
  _In_ SIZE_T BytecodeLength,
  _Out_opt_ ID3D10VertexShader** ppVertexShader)
{
  return device_->CreateVertexShader(
    pShaderBytecode, BytecodeLength, ppVertexShader);
}

HRESULT WINAPI D3D10DeviceProxy::CreateGeometryShader(
  _In_reads_(BytecodeLength) const void* pShaderBytecode,
  _In_ SIZE_T BytecodeLength,
  _Out_opt_ ID3D10GeometryShader** ppGeometryShader)
{
  return device_->CreateGeometryShader(
    pShaderBytecode, BytecodeLength, ppGeometryShader);
}

HRESULT WINAPI D3D10DeviceProxy::CreateGeometryShaderWithStreamOutput(
  _In_reads_(BytecodeLength) const void* pShaderBytecode,
  _In_ SIZE_T BytecodeLength,
  _In_reads_opt_(NumEntries) const D3D10_SO_DECLARATION_ENTRY* pSODeclaration,
  _In_range_(0, D3D10_SO_SINGLE_BUFFER_COMPONENT_LIMIT) UINT NumEntries,
  _In_ UINT OutputStreamStride,
  _Out_opt_ ID3D10GeometryShader** ppGeometryShader)
{
  return device_->CreateGeometryShaderWithStreamOutput(pShaderBytecode,
                                                       BytecodeLength,
                                                       pSODeclaration,
                                                       NumEntries,
                                                       OutputStreamStride,
                                                       ppGeometryShader);
}

HRESULT WINAPI D3D10DeviceProxy::CreatePixelShader(
  _In_reads_(BytecodeLength) const void* pShaderBytecode,
  _In_ SIZE_T BytecodeLength,
  _Out_opt_ ID3D10PixelShader** ppPixelShader)
{
  return device_->CreatePixelShader(
    pShaderBytecode, BytecodeLength, ppPixelShader);
}

HRESULT WINAPI D3D10DeviceProxy::CreateBlendState(
  _In_ const D3D10_BLEND_DESC* pBlendStateDesc,
  _Out_opt_ ID3D10BlendState** ppBlendState)
{
  return device_->CreateBlendState(pBlendStateDesc, ppBlendState);
}

HRESULT WINAPI D3D10DeviceProxy::CreateDepthStencilState(
  _In_ const D3D10_DEPTH_STENCIL_DESC* pDepthStencilDesc,
  _Out_opt_ ID3D10DepthStencilState** ppDepthStencilState)
{
  return device_->CreateDepthStencilState(pDepthStencilDesc,
                                          ppDepthStencilState);
}

HRESULT WINAPI D3D10DeviceProxy::CreateRasterizerState(
  _In_ const D3D10_RASTERIZER_DESC* pRasterizerDesc,
  _Out_opt_ ID3D10RasterizerState** ppRasterizerState)
{
  return device_->CreateRasterizerState(pRasterizerDesc, ppRasterizerState);
}

HRESULT WINAPI D3D10DeviceProxy::CreateSamplerState(
  _In_ const D3D10_SAMPLER_DESC* pSamplerDesc,
  _Out_opt_ ID3D10SamplerState** ppSamplerState)
{
  return device_->CreateSamplerState(pSamplerDesc, ppSamplerState);
}

HRESULT WINAPI
  D3D10DeviceProxy::CreateQuery(_In_ const D3D10_QUERY_DESC* pQueryDesc,
                                _Out_opt_ ID3D10Query** ppQuery)
{
  return device_->CreateQuery(pQueryDesc, ppQuery);
}

HRESULT WINAPI
  D3D10DeviceProxy::CreatePredicate(_In_ const D3D10_QUERY_DESC* pPredicateDesc,
                                    _Out_opt_ ID3D10Predicate** ppPredicate)
{
  return device_->CreatePredicate(pPredicateDesc, ppPredicate);
}

HRESULT WINAPI
  D3D10DeviceProxy::CreateCounter(_In_ const D3D10_COUNTER_DESC* pCounterDesc,
                                  _Out_opt_ ID3D10Counter** ppCounter)
{
  return device_->CreateCounter(pCounterDesc, ppCounter);
}

HRESULT WINAPI D3D10DeviceProxy::CheckFormatSupport(_In_ DXGI_FORMAT Format,
                                                    _Out_ UINT* pFormatSupport)
{
  return device_->CheckFormatSupport(Format, pFormatSupport);
}

HRESULT WINAPI
  D3D10DeviceProxy::CheckMultisampleQualityLevels(_In_ DXGI_FORMAT Format,
                                                  _In_ UINT SampleCount,
                                                  _Out_ UINT* pNumQualityLevels)
{
  return device_->CheckMultisampleQualityLevels(
    Format, SampleCount, pNumQualityLevels);
}

void WINAPI
  D3D10DeviceProxy::CheckCounterInfo(_Out_ D3D10_COUNTER_INFO* pCounterInfo)
{
  return device_->CheckCounterInfo(pCounterInfo);
}

HRESULT WINAPI
  D3D10DeviceProxy::CheckCounter(_In_ const D3D10_COUNTER_DESC* pDesc,
                                 _Out_ D3D10_COUNTER_TYPE* pType,
                                 _Out_ UINT* pActiveCounters,
                                 _Out_writes_opt_(*pNameLength) LPSTR szName,
                                 _Inout_opt_ UINT* pNameLength,
                                 _Out_writes_opt_(*pUnitsLength) LPSTR szUnits,
                                 _Inout_opt_ UINT* pUnitsLength,
                                 _Out_writes_opt_(*pDescriptionLength)
                                   LPSTR szDescription,
                                 _Inout_opt_ UINT* pDescriptionLength)
{
  return device_->CheckCounter(pDesc,
                               pType,
                               pActiveCounters,
                               szName,
                               pNameLength,
                               szUnits,
                               pUnitsLength,
                               szDescription,
                               pDescriptionLength);
}

UINT WINAPI D3D10DeviceProxy::GetCreationFlags()
{
  return device_->GetCreationFlags();
}

HRESULT WINAPI
  D3D10DeviceProxy::OpenSharedResource(_In_ HANDLE hResource,
                                       _In_ REFIID ReturnedInterface,
                                       _Out_opt_ void** ppResource)
{
  return device_->OpenSharedResource(hResource, ReturnedInterface, ppResource);
}

void WINAPI
  D3D10DeviceProxy::SetTextFilterSize(_In_ UINT Width, _In_ UINT Height)
{
  return device_->SetTextFilterSize(Width, Height);
}

void WINAPI D3D10DeviceProxy::GetTextFilterSize(_Out_opt_ UINT* pWidth,
                                                _Out_opt_ UINT* pHeight)
{
  return device_->GetTextFilterSize(pWidth, pHeight);
}

#if !defined(HADESMEM_GCC)
// ID3D10Device1
HRESULT WINAPI D3D10DeviceProxy::CreateShaderResourceView1(
  _In_ ID3D10Resource* pResource,
  _In_opt_ const D3D10_SHADER_RESOURCE_VIEW_DESC1* pDesc,
  _Out_opt_ ID3D10ShaderResourceView1** ppSRView)
{
  auto const device = static_cast<ID3D10Device1*>(device_);
  return device->CreateShaderResourceView1(pResource, pDesc, ppSRView);
}

HRESULT WINAPI D3D10DeviceProxy::CreateBlendState1(
  _In_ const D3D10_BLEND_DESC1* pBlendStateDesc,
  _Out_opt_ ID3D10BlendState1** ppBlendState)
{
  auto const device = static_cast<ID3D10Device1*>(device_);
  return device->CreateBlendState1(pBlendStateDesc, ppBlendState);
}

D3D10_FEATURE_LEVEL1 WINAPI D3D10DeviceProxy::GetFeatureLevel()
{
  auto const device = static_cast<ID3D10Device1*>(device_);
  return device->GetFeatureLevel();
}
#endif // #if !defined(HADESMEM_GCC)

void D3D10DeviceProxy::Cleanup()
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  auto& callbacks = GetOnReleaseD3D10Callbacks();
  callbacks.Run(device_);
}
}
}
