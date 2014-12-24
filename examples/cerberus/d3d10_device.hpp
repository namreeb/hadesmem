// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>

#include <windows.h>

#include <d3d10_1.h>
#include <d3d10.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif // #if defined(HADESMEM_GCC)

class D3D10DeviceProxy : public ID3D10Device1
{
public:
  explicit D3D10DeviceProxy(ID3D10Device* device)
    : device_{static_cast<ID3D10Device1*>(device)}
  {
  }

  // IUnknown
  virtual HRESULT WINAPI QueryInterface(REFIID riid, void** obj) override;
  virtual ULONG WINAPI AddRef() override;
  virtual ULONG WINAPI Release() override;

  // ID3D10Device
  virtual void STDMETHODCALLTYPE VSSetConstantBuffers(
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
      UINT NumBuffers,
    _In_reads_opt_(NumBuffers) ID3D10Buffer* const* ppConstantBuffers) override;
  virtual void STDMETHODCALLTYPE PSSetShaderResources(
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
      UINT NumViews,
    _In_reads_opt_(NumViews)
      ID3D10ShaderResourceView* const* ppShaderResourceViews) override;
  virtual void STDMETHODCALLTYPE
    PSSetShader(_In_opt_ ID3D10PixelShader* pPixelShader) override;
  virtual void STDMETHODCALLTYPE PSSetSamplers(
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
      UINT NumSamplers,
    _In_reads_opt_(NumSamplers) ID3D10SamplerState* const* ppSamplers) override;
  virtual void STDMETHODCALLTYPE
    VSSetShader(_In_opt_ ID3D10VertexShader* pVertexShader) override;
  virtual void STDMETHODCALLTYPE
    DrawIndexed(_In_ UINT IndexCount,
                _In_ UINT StartIndexLocation,
                _In_ INT BaseVertexLocation) override;
  virtual void STDMETHODCALLTYPE
    Draw(_In_ UINT VertexCount, _In_ UINT StartVertexLocation) override;
  virtual void STDMETHODCALLTYPE PSSetConstantBuffers(
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
      UINT NumBuffers,
    _In_reads_opt_(NumBuffers) ID3D10Buffer* const* ppConstantBuffers) override;
  virtual void STDMETHODCALLTYPE
    IASetInputLayout(_In_opt_ ID3D10InputLayout* pInputLayout) override;
  virtual void STDMETHODCALLTYPE IASetVertexBuffers(
    _In_range_(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
      UINT NumBuffers,
    _In_reads_opt_(NumBuffers) ID3D10Buffer* const* ppVertexBuffers,
    _In_reads_opt_(NumBuffers) const UINT* pStrides,
    _In_reads_opt_(NumBuffers) const UINT* pOffsets) override;
  virtual void STDMETHODCALLTYPE
    IASetIndexBuffer(_In_opt_ ID3D10Buffer* pIndexBuffer,
                     _In_ DXGI_FORMAT Format,
                     _In_ UINT Offset) override;
  virtual void STDMETHODCALLTYPE
    DrawIndexedInstanced(_In_ UINT IndexCountPerInstance,
                         _In_ UINT InstanceCount,
                         _In_ UINT StartIndexLocation,
                         _In_ INT BaseVertexLocation,
                         _In_ UINT StartInstanceLocation) override;
  virtual void STDMETHODCALLTYPE
    DrawInstanced(_In_ UINT VertexCountPerInstance,
                  _In_ UINT InstanceCount,
                  _In_ UINT StartVertexLocation,
                  _In_ UINT StartInstanceLocation) override;
  virtual void STDMETHODCALLTYPE GSSetConstantBuffers(
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
      UINT NumBuffers,
    _In_reads_opt_(NumBuffers) ID3D10Buffer* const* ppConstantBuffers) override;
  virtual void STDMETHODCALLTYPE
    GSSetShader(_In_opt_ ID3D10GeometryShader* pShader) override;
  virtual void STDMETHODCALLTYPE
    IASetPrimitiveTopology(_In_ D3D10_PRIMITIVE_TOPOLOGY Topology) override;
  virtual void STDMETHODCALLTYPE VSSetShaderResources(
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
      UINT NumViews,
    _In_reads_opt_(NumViews)
      ID3D10ShaderResourceView* const* ppShaderResourceViews) override;
  virtual void STDMETHODCALLTYPE VSSetSamplers(
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
      UINT NumSamplers,
    _In_reads_opt_(NumSamplers) ID3D10SamplerState* const* ppSamplers) override;
  virtual void STDMETHODCALLTYPE
    SetPredication(_In_opt_ ID3D10Predicate* pPredicate,
                   _In_ BOOL PredicateValue) override;
  virtual void STDMETHODCALLTYPE GSSetShaderResources(
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
      UINT NumViews,
    _In_reads_opt_(NumViews)
      ID3D10ShaderResourceView* const* ppShaderResourceViews) override;
  virtual void STDMETHODCALLTYPE GSSetSamplers(
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
      UINT NumSamplers,
    _In_reads_opt_(NumSamplers) ID3D10SamplerState* const* ppSamplers) override;
  virtual void STDMETHODCALLTYPE OMSetRenderTargets(
    _In_range_(0, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumViews,
    _In_reads_opt_(NumViews) ID3D10RenderTargetView* const* ppRenderTargetViews,
    _In_opt_ ID3D10DepthStencilView* pDepthStencilView) override;
  virtual void STDMETHODCALLTYPE
    OMSetBlendState(_In_opt_ ID3D10BlendState* pBlendState,
                    _In_ const FLOAT BlendFactor[4],
                    _In_ UINT SampleMask) override;
  virtual void STDMETHODCALLTYPE
    OMSetDepthStencilState(_In_opt_ ID3D10DepthStencilState* pDepthStencilState,
                           _In_ UINT StencilRef) override;
  virtual void STDMETHODCALLTYPE
    SOSetTargets(_In_range_(0, D3D10_SO_BUFFER_SLOT_COUNT) UINT NumBuffers,
                 _In_reads_opt_(NumBuffers) ID3D10Buffer* const* ppSOTargets,
                 _In_reads_opt_(NumBuffers) const UINT* pOffsets) override;
  virtual void STDMETHODCALLTYPE DrawAuto() override;
  virtual void STDMETHODCALLTYPE
    RSSetState(_In_opt_ ID3D10RasterizerState* pRasterizerState) override;
  virtual void STDMETHODCALLTYPE RSSetViewports(
    _In_range_(0, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)
      UINT NumViewports,
    _In_reads_opt_(NumViewports) const D3D10_VIEWPORT* pViewports) override;
  virtual void STDMETHODCALLTYPE RSSetScissorRects(
    _In_range_(0, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)
      UINT NumRects,
    _In_reads_opt_(NumRects) const D3D10_RECT* pRects) override;
  virtual void STDMETHODCALLTYPE
    CopySubresourceRegion(_In_ ID3D10Resource* pDstResource,
                          _In_ UINT DstSubresource,
                          _In_ UINT DstX,
                          _In_ UINT DstY,
                          _In_ UINT DstZ,
                          _In_ ID3D10Resource* pSrcResource,
                          _In_ UINT SrcSubresource,
                          _In_opt_ const D3D10_BOX* pSrcBox) override;
  virtual void STDMETHODCALLTYPE
    CopyResource(_In_ ID3D10Resource* pDstResource,
                 _In_ ID3D10Resource* pSrcResource) override;
  virtual void STDMETHODCALLTYPE
    UpdateSubresource(_In_ ID3D10Resource* pDstResource,
                      _In_ UINT DstSubresource,
                      _In_opt_ const D3D10_BOX* pDstBox,
                      _In_ const void* pSrcData,
                      _In_ UINT SrcRowPitch,
                      _In_ UINT SrcDepthPitch) override;
  virtual void STDMETHODCALLTYPE
    ClearRenderTargetView(_In_ ID3D10RenderTargetView* pRenderTargetView,
                          _In_ const FLOAT ColorRGBA[4]) override;
  virtual void STDMETHODCALLTYPE
    ClearDepthStencilView(_In_ ID3D10DepthStencilView* pDepthStencilView,
                          _In_ UINT ClearFlags,
                          _In_ FLOAT Depth,
                          _In_ UINT8 Stencil) override;
  virtual void STDMETHODCALLTYPE
    GenerateMips(_In_ ID3D10ShaderResourceView* pShaderResourceView) override;
  virtual void STDMETHODCALLTYPE
    ResolveSubresource(_In_ ID3D10Resource* pDstResource,
                       _In_ UINT DstSubresource,
                       _In_ ID3D10Resource* pSrcResource,
                       _In_ UINT SrcSubresource,
                       _In_ DXGI_FORMAT Format) override;
  virtual void STDMETHODCALLTYPE VSGetConstantBuffers(
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
      UINT NumBuffers,
    _Out_writes_opt_(NumBuffers) ID3D10Buffer** ppConstantBuffers) override;
  virtual void STDMETHODCALLTYPE PSGetShaderResources(
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
      UINT NumViews,
    _Out_writes_opt_(NumViews)
      ID3D10ShaderResourceView** ppShaderResourceViews) override;
  virtual void STDMETHODCALLTYPE
    PSGetShader(_Out_ ID3D10PixelShader** ppPixelShader) override;
  virtual void STDMETHODCALLTYPE PSGetSamplers(
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
      UINT NumSamplers,
    _Out_writes_opt_(NumSamplers) ID3D10SamplerState** ppSamplers) override;
  virtual void STDMETHODCALLTYPE
    VSGetShader(_Out_ ID3D10VertexShader** ppVertexShader) override;
  virtual void STDMETHODCALLTYPE PSGetConstantBuffers(
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
      UINT NumBuffers,
    _Out_writes_opt_(NumBuffers) ID3D10Buffer** ppConstantBuffers) override;
  virtual void STDMETHODCALLTYPE
    IAGetInputLayout(_Out_ ID3D10InputLayout** ppInputLayout) override;
  virtual void STDMETHODCALLTYPE IAGetVertexBuffers(
    _In_range_(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_1_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
      UINT NumBuffers,
    _Out_writes_opt_(NumBuffers) ID3D10Buffer** ppVertexBuffers,
    _Out_writes_opt_(NumBuffers) UINT* pStrides,
    _Out_writes_opt_(NumBuffers) UINT* pOffsets) override;
  virtual void STDMETHODCALLTYPE
    IAGetIndexBuffer(_Out_opt_ ID3D10Buffer** pIndexBuffer,
                     _Out_opt_ DXGI_FORMAT* Format,
                     _Out_opt_ UINT* Offset) override;
  virtual void STDMETHODCALLTYPE GSGetConstantBuffers(
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)
      UINT NumBuffers,
    _Out_writes_opt_(NumBuffers) ID3D10Buffer** ppConstantBuffers) override;
  virtual void STDMETHODCALLTYPE
    GSGetShader(_Out_ ID3D10GeometryShader** ppGeometryShader) override;
  virtual void STDMETHODCALLTYPE
    IAGetPrimitiveTopology(_Out_ D3D10_PRIMITIVE_TOPOLOGY* pTopology) override;
  virtual void STDMETHODCALLTYPE VSGetShaderResources(
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
      UINT NumViews,
    _Out_writes_opt_(NumViews)
      ID3D10ShaderResourceView** ppShaderResourceViews) override;
  virtual void STDMETHODCALLTYPE VSGetSamplers(
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
      UINT NumSamplers,
    _Out_writes_opt_(NumSamplers) ID3D10SamplerState** ppSamplers) override;
  virtual void STDMETHODCALLTYPE
    GetPredication(_Out_opt_ ID3D10Predicate** ppPredicate,
                   _Out_opt_ BOOL* pPredicateValue) override;
  virtual void STDMETHODCALLTYPE GSGetShaderResources(
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)
      UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)
      UINT NumViews,
    _Out_writes_opt_(NumViews)
      ID3D10ShaderResourceView** ppShaderResourceViews) override;
  virtual void STDMETHODCALLTYPE GSGetSamplers(
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot,
    _In_range_(0, D3D10_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)
      UINT NumSamplers,
    _Out_writes_opt_(NumSamplers) ID3D10SamplerState** ppSamplers) override;
  virtual void STDMETHODCALLTYPE OMGetRenderTargets(
    _In_range_(0, D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumViews,
    _Out_writes_opt_(NumViews) ID3D10RenderTargetView** ppRenderTargetViews,
    _Out_opt_ ID3D10DepthStencilView** ppDepthStencilView) override;
  virtual void STDMETHODCALLTYPE
    OMGetBlendState(_Out_opt_ ID3D10BlendState** ppBlendState,
                    _Out_opt_ FLOAT BlendFactor[4],
                    _Out_opt_ UINT* pSampleMask) override;
  virtual void STDMETHODCALLTYPE OMGetDepthStencilState(
    _Out_opt_ ID3D10DepthStencilState** ppDepthStencilState,
    _Out_opt_ UINT* pStencilRef) override;
  virtual void STDMETHODCALLTYPE
    SOGetTargets(_In_range_(0, D3D10_SO_BUFFER_SLOT_COUNT) UINT NumBuffers,
                 _Out_writes_opt_(NumBuffers) ID3D10Buffer** ppSOTargets,
                 _Out_writes_opt_(NumBuffers) UINT* pOffsets) override;
  virtual void STDMETHODCALLTYPE
    RSGetState(_Out_ ID3D10RasterizerState** ppRasterizerState) override;
  virtual void STDMETHODCALLTYPE RSGetViewports(
    _Inout_ /*_range(0, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/ UINT*
      NumViewports,
    _Out_writes_opt_(*NumViewports) D3D10_VIEWPORT* pViewports) override;
  virtual void STDMETHODCALLTYPE RSGetScissorRects(
    _Inout_ /*_range(0, D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/ UINT*
      NumRects,
    _Out_writes_opt_(*NumRects) D3D10_RECT* pRects) override;
  virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason() override;
  virtual HRESULT STDMETHODCALLTYPE SetExceptionMode(UINT RaiseFlags) override;
  virtual UINT STDMETHODCALLTYPE GetExceptionMode() override;
  virtual HRESULT STDMETHODCALLTYPE
    GetPrivateData(_In_ REFGUID guid,
                   _Inout_ UINT* pDataSize,
                   _Out_writes_bytes_opt_(*pDataSize) void* pData) override;
  virtual HRESULT STDMETHODCALLTYPE
    SetPrivateData(_In_ REFGUID guid,
                   _In_ UINT DataSize,
                   _In_reads_bytes_opt_(DataSize) const void* pData) override;
  virtual HRESULT STDMETHODCALLTYPE
    SetPrivateDataInterface(_In_ REFGUID guid,
                            _In_opt_ const IUnknown* pData) override;
  virtual void STDMETHODCALLTYPE ClearState() override;
  virtual void STDMETHODCALLTYPE Flush() override;
  virtual HRESULT STDMETHODCALLTYPE
    CreateBuffer(_In_ const D3D10_BUFFER_DESC* pDesc,
                 _In_opt_ const D3D10_SUBRESOURCE_DATA* pInitialData,
                 _Out_opt_ ID3D10Buffer** ppBuffer) override;
  virtual HRESULT STDMETHODCALLTYPE CreateTexture1D(
    _In_ const D3D10_TEXTURE1D_DESC* pDesc,
    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))
      const D3D10_SUBRESOURCE_DATA* pInitialData,
    _Out_ ID3D10Texture1D** ppTexture1D) override;
  virtual HRESULT STDMETHODCALLTYPE CreateTexture2D(
    _In_ const D3D10_TEXTURE2D_DESC* pDesc,
    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))
      const D3D10_SUBRESOURCE_DATA* pInitialData,
    _Out_ ID3D10Texture2D** ppTexture2D) override;
  virtual HRESULT STDMETHODCALLTYPE
    CreateTexture3D(_In_ const D3D10_TEXTURE3D_DESC* pDesc,
                    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))
                      const D3D10_SUBRESOURCE_DATA* pInitialData,
                    _Out_ ID3D10Texture3D** ppTexture3D) override;
  virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView(
    _In_ ID3D10Resource* pResource,
    _In_opt_ const D3D10_SHADER_RESOURCE_VIEW_DESC* pDesc,
    _Out_opt_ ID3D10ShaderResourceView** ppSRView) override;
  virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView(
    _In_ ID3D10Resource* pResource,
    _In_opt_ const D3D10_RENDER_TARGET_VIEW_DESC* pDesc,
    _Out_opt_ ID3D10RenderTargetView** ppRTView) override;
  virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView(
    _In_ ID3D10Resource* pResource,
    _In_opt_ const D3D10_DEPTH_STENCIL_VIEW_DESC* pDesc,
    _Out_opt_ ID3D10DepthStencilView** ppDepthStencilView) override;
  virtual HRESULT STDMETHODCALLTYPE CreateInputLayout(
    _In_reads_(NumElements) const D3D10_INPUT_ELEMENT_DESC* pInputElementDescs,
    _In_range_(0, D3D10_1_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)
      UINT NumElements,
    _In_reads_(BytecodeLength) const void* pShaderBytecodeWithInputSignature,
    _In_ SIZE_T BytecodeLength,
    _Out_opt_ ID3D10InputLayout** ppInputLayout) override;
  virtual HRESULT STDMETHODCALLTYPE
    CreateVertexShader(_In_reads_(BytecodeLength) const void* pShaderBytecode,
                       _In_ SIZE_T BytecodeLength,
                       _Out_opt_ ID3D10VertexShader** ppVertexShader) override;
  virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader(
    _In_reads_(BytecodeLength) const void* pShaderBytecode,
    _In_ SIZE_T BytecodeLength,
    _Out_opt_ ID3D10GeometryShader** ppGeometryShader) override;
  virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput(
    _In_reads_(BytecodeLength) const void* pShaderBytecode,
    _In_ SIZE_T BytecodeLength,
    _In_reads_opt_(NumEntries) const D3D10_SO_DECLARATION_ENTRY* pSODeclaration,
    _In_range_(0, D3D10_SO_SINGLE_BUFFER_COMPONENT_LIMIT) UINT NumEntries,
    _In_ UINT OutputStreamStride,
    _Out_opt_ ID3D10GeometryShader** ppGeometryShader) override;
  virtual HRESULT STDMETHODCALLTYPE
    CreatePixelShader(_In_reads_(BytecodeLength) const void* pShaderBytecode,
                      _In_ SIZE_T BytecodeLength,
                      _Out_opt_ ID3D10PixelShader** ppPixelShader) override;
  virtual HRESULT STDMETHODCALLTYPE
    CreateBlendState(_In_ const D3D10_BLEND_DESC* pBlendStateDesc,
                     _Out_opt_ ID3D10BlendState** ppBlendState) override;
  virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState(
    _In_ const D3D10_DEPTH_STENCIL_DESC* pDepthStencilDesc,
    _Out_opt_ ID3D10DepthStencilState** ppDepthStencilState) override;
  virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState(
    _In_ const D3D10_RASTERIZER_DESC* pRasterizerDesc,
    _Out_opt_ ID3D10RasterizerState** ppRasterizerState) override;
  virtual HRESULT STDMETHODCALLTYPE
    CreateSamplerState(_In_ const D3D10_SAMPLER_DESC* pSamplerDesc,
                       _Out_opt_ ID3D10SamplerState** ppSamplerState) override;
  virtual HRESULT STDMETHODCALLTYPE
    CreateQuery(_In_ const D3D10_QUERY_DESC* pQueryDesc,
                _Out_opt_ ID3D10Query** ppQuery) override;
  virtual HRESULT STDMETHODCALLTYPE
    CreatePredicate(_In_ const D3D10_QUERY_DESC* pPredicateDesc,
                    _Out_opt_ ID3D10Predicate** ppPredicate) override;
  virtual HRESULT STDMETHODCALLTYPE
    CreateCounter(_In_ const D3D10_COUNTER_DESC* pCounterDesc,
                  _Out_opt_ ID3D10Counter** ppCounter) override;
  virtual HRESULT STDMETHODCALLTYPE
    CheckFormatSupport(_In_ DXGI_FORMAT Format,
                       _Out_ UINT* pFormatSupport) override;
  virtual HRESULT STDMETHODCALLTYPE
    CheckMultisampleQualityLevels(_In_ DXGI_FORMAT Format,
                                  _In_ UINT SampleCount,
                                  _Out_ UINT* pNumQualityLevels) override;
  virtual void STDMETHODCALLTYPE
    CheckCounterInfo(_Out_ D3D10_COUNTER_INFO* pCounterInfo) override;
  virtual HRESULT STDMETHODCALLTYPE
    CheckCounter(_In_ const D3D10_COUNTER_DESC* pDesc,
                 _Out_ D3D10_COUNTER_TYPE* pType,
                 _Out_ UINT* pActiveCounters,
                 _Out_writes_opt_(*pNameLength) LPSTR szName,
                 _Inout_opt_ UINT* pNameLength,
                 _Out_writes_opt_(*pUnitsLength) LPSTR szUnits,
                 _Inout_opt_ UINT* pUnitsLength,
                 _Out_writes_opt_(*pDescriptionLength) LPSTR szDescription,
                 _Inout_opt_ UINT* pDescriptionLength) override;
  virtual UINT STDMETHODCALLTYPE GetCreationFlags() override;
  virtual HRESULT STDMETHODCALLTYPE
    OpenSharedResource(_In_ HANDLE hResource,
                       _In_ REFIID ReturnedInterface,
                       _Out_opt_ void** ppResource) override;
  virtual void STDMETHODCALLTYPE
    SetTextFilterSize(_In_ UINT Width, _In_ UINT Height) override;
  virtual void STDMETHODCALLTYPE
    GetTextFilterSize(_Out_opt_ UINT* pWidth, _Out_opt_ UINT* pHeight) override;

  // ID3D10Device1
  virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView1(
    _In_ ID3D10Resource* pResource,
    _In_opt_ const D3D10_SHADER_RESOURCE_VIEW_DESC1* pDesc,
    _Out_opt_ ID3D10ShaderResourceView1** ppSRView) override;
  virtual HRESULT STDMETHODCALLTYPE
    CreateBlendState1(_In_ const D3D10_BLEND_DESC1* pBlendStateDesc,
                      _Out_opt_ ID3D10BlendState1** ppBlendState) override;
  virtual D3D10_FEATURE_LEVEL1 STDMETHODCALLTYPE GetFeatureLevel() override;

protected:
  void Cleanup();

  std::int64_t refs_{1};
  ID3D10Device1* device_{};
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)
}
}
