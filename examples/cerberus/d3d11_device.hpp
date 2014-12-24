// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>

#include <windows.h>

#include <d3d11.h>
#include <d3d11_2.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif // #if defined(HADESMEM_GCC)

class D3D11DeviceProxy : public ID3D11Device2
{
public:
  explicit D3D11DeviceProxy(ID3D11Device* device)
    : device_{static_cast<ID3D11Device2*>(device)}
  {
  }

  // IUnknown
  virtual HRESULT WINAPI QueryInterface(REFIID riid, void** obj) override;
  virtual ULONG WINAPI AddRef() override;
  virtual ULONG WINAPI Release() override;

  // ID3D11Device
  virtual HRESULT WINAPI
    CreateBuffer(_In_ const D3D11_BUFFER_DESC* pDesc,
                 _In_opt_ const D3D11_SUBRESOURCE_DATA* pInitialData,
                 _Out_opt_ ID3D11Buffer** ppBuffer) override;
  virtual HRESULT WINAPI CreateTexture1D(
    _In_ const D3D11_TEXTURE1D_DESC* pDesc,
    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))
      const D3D11_SUBRESOURCE_DATA* pInitialData,
    _Out_opt_ ID3D11Texture1D** ppTexture1D) override;
  virtual HRESULT WINAPI CreateTexture2D(
    _In_ const D3D11_TEXTURE2D_DESC* pDesc,
    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))
      const D3D11_SUBRESOURCE_DATA* pInitialData,
    _Out_opt_ ID3D11Texture2D** ppTexture2D) override;
  virtual HRESULT WINAPI
    CreateTexture3D(_In_ const D3D11_TEXTURE3D_DESC* pDesc,
                    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))
                      const D3D11_SUBRESOURCE_DATA* pInitialData,
                    _Out_opt_ ID3D11Texture3D** ppTexture3D) override;
  virtual HRESULT WINAPI CreateShaderResourceView(
    _In_ ID3D11Resource* pResource,
    _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
    _Out_opt_ ID3D11ShaderResourceView** ppSRView) override;
  virtual HRESULT WINAPI CreateUnorderedAccessView(
    _In_ ID3D11Resource* pResource,
    _In_opt_ const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
    _Out_opt_ ID3D11UnorderedAccessView** ppUAView) override;
  virtual HRESULT WINAPI CreateRenderTargetView(
    _In_ ID3D11Resource* pResource,
    _In_opt_ const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
    _Out_opt_ ID3D11RenderTargetView** ppRTView) override;
  virtual HRESULT WINAPI CreateDepthStencilView(
    _In_ ID3D11Resource* pResource,
    _In_opt_ const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
    _Out_opt_ ID3D11DepthStencilView** ppDepthStencilView) override;
  virtual HRESULT WINAPI CreateInputLayout(
    _In_reads_(NumElements) const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
    _In_range_(0, D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)
      UINT NumElements,
    _In_reads_(BytecodeLength) const void* pShaderBytecodeWithInputSignature,
    _In_ SIZE_T BytecodeLength,
    _Out_opt_ ID3D11InputLayout** ppInputLayout) override;
  virtual HRESULT WINAPI
    CreateVertexShader(_In_reads_(BytecodeLength) const void* pShaderBytecode,
                       _In_ SIZE_T BytecodeLength,
                       _In_opt_ ID3D11ClassLinkage* pClassLinkage,
                       _Out_opt_ ID3D11VertexShader** ppVertexShader) override;
  virtual HRESULT WINAPI CreateGeometryShader(
    _In_reads_(BytecodeLength) const void* pShaderBytecode,
    _In_ SIZE_T BytecodeLength,
    _In_opt_ ID3D11ClassLinkage* pClassLinkage,
    _Out_opt_ ID3D11GeometryShader** ppGeometryShader) override;
  virtual HRESULT WINAPI CreateGeometryShaderWithStreamOutput(
    _In_reads_(BytecodeLength) const void* pShaderBytecode,
    _In_ SIZE_T BytecodeLength,
    _In_reads_opt_(NumEntries) const D3D11_SO_DECLARATION_ENTRY* pSODeclaration,
    _In_range_(0, D3D11_SO_STREAM_COUNT* D3D11_SO_OUTPUT_COMPONENT_COUNT)
      UINT NumEntries,
    _In_reads_opt_(NumStrides) const UINT* pBufferStrides,
    _In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT) UINT NumStrides,
    _In_ UINT RasterizedStream,
    _In_opt_ ID3D11ClassLinkage* pClassLinkage,
    _Out_opt_ ID3D11GeometryShader** ppGeometryShader) override;
  virtual HRESULT WINAPI
    CreatePixelShader(_In_reads_(BytecodeLength) const void* pShaderBytecode,
                      _In_ SIZE_T BytecodeLength,
                      _In_opt_ ID3D11ClassLinkage* pClassLinkage,
                      _Out_opt_ ID3D11PixelShader** ppPixelShader) override;
  virtual HRESULT WINAPI
    CreateHullShader(_In_reads_(BytecodeLength) const void* pShaderBytecode,
                     _In_ SIZE_T BytecodeLength,
                     _In_opt_ ID3D11ClassLinkage* pClassLinkage,
                     _Out_opt_ ID3D11HullShader** ppHullShader) override;
  virtual HRESULT WINAPI
    CreateDomainShader(_In_reads_(BytecodeLength) const void* pShaderBytecode,
                       _In_ SIZE_T BytecodeLength,
                       _In_opt_ ID3D11ClassLinkage* pClassLinkage,
                       _Out_opt_ ID3D11DomainShader** ppDomainShader) override;
  virtual HRESULT WINAPI CreateComputeShader(
    _In_reads_(BytecodeLength) const void* pShaderBytecode,
    _In_ SIZE_T BytecodeLength,
    _In_opt_ ID3D11ClassLinkage* pClassLinkage,
    _Out_opt_ ID3D11ComputeShader** ppComputeShader) override;
  virtual HRESULT WINAPI
    CreateClassLinkage(_Out_ ID3D11ClassLinkage** ppLinkage) override;
  virtual HRESULT WINAPI
    CreateBlendState(_In_ const D3D11_BLEND_DESC* pBlendStateDesc,
                     _Out_opt_ ID3D11BlendState** ppBlendState) override;
  virtual HRESULT WINAPI CreateDepthStencilState(
    _In_ const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
    _Out_opt_ ID3D11DepthStencilState** ppDepthStencilState) override;
  virtual HRESULT WINAPI CreateRasterizerState(
    _In_ const D3D11_RASTERIZER_DESC* pRasterizerDesc,
    _Out_opt_ ID3D11RasterizerState** ppRasterizerState) override;
  virtual HRESULT WINAPI
    CreateSamplerState(_In_ const D3D11_SAMPLER_DESC* pSamplerDesc,
                       _Out_opt_ ID3D11SamplerState** ppSamplerState) override;
  virtual HRESULT WINAPI CreateQuery(_In_ const D3D11_QUERY_DESC* pQueryDesc,
                                     _Out_opt_ ID3D11Query** ppQuery) override;
  virtual HRESULT WINAPI
    CreatePredicate(_In_ const D3D11_QUERY_DESC* pPredicateDesc,
                    _Out_opt_ ID3D11Predicate** ppPredicate) override;
  virtual HRESULT WINAPI
    CreateCounter(_In_ const D3D11_COUNTER_DESC* pCounterDesc,
                  _Out_opt_ ID3D11Counter** ppCounter) override;
  virtual HRESULT WINAPI CreateDeferredContext(
    UINT ContextFlags,
    _Out_opt_ ID3D11DeviceContext** ppDeferredContext) override;
  virtual HRESULT WINAPI
    OpenSharedResource(_In_ HANDLE hResource,
                       _In_ REFIID ReturnedInterface,
                       _Out_opt_ void** ppResource) override;
  virtual HRESULT WINAPI
    CheckFormatSupport(_In_ DXGI_FORMAT Format,
                       _Out_ UINT* pFormatSupport) override;
  virtual HRESULT WINAPI
    CheckMultisampleQualityLevels(_In_ DXGI_FORMAT Format,
                                  _In_ UINT SampleCount,
                                  _Out_ UINT* pNumQualityLevels) override;
  virtual void WINAPI
    CheckCounterInfo(_Out_ D3D11_COUNTER_INFO* pCounterInfo) override;
  virtual HRESULT WINAPI
    CheckCounter(_In_ const D3D11_COUNTER_DESC* pDesc,
                 _Out_ D3D11_COUNTER_TYPE* pType,
                 _Out_ UINT* pActiveCounters,
                 _Out_writes_opt_(*pNameLength) LPSTR szName,
                 _Inout_opt_ UINT* pNameLength,
                 _Out_writes_opt_(*pUnitsLength) LPSTR szUnits,
                 _Inout_opt_ UINT* pUnitsLength,
                 _Out_writes_opt_(*pDescriptionLength) LPSTR szDescription,
                 _Inout_opt_ UINT* pDescriptionLength) override;
  virtual HRESULT WINAPI CheckFeatureSupport(
    D3D11_FEATURE Feature,
    _Out_writes_bytes_(FeatureSupportDataSize) void* pFeatureSupportData,
    UINT FeatureSupportDataSize) override;
  virtual HRESULT WINAPI
    GetPrivateData(_In_ REFGUID guid,
                   _Inout_ UINT* pDataSize,
                   _Out_writes_bytes_opt_(*pDataSize) void* pData) override;
  virtual HRESULT WINAPI SetPrivateData(_In_ REFGUID guid,
                                        _In_ UINT DataSize,
                                        _In_reads_bytes_opt_(DataSize)
                                          const void* pData) override;
  virtual HRESULT WINAPI
    SetPrivateDataInterface(_In_ REFGUID guid,
                            _In_opt_ const IUnknown* pData) override;
  virtual D3D_FEATURE_LEVEL WINAPI GetFeatureLevel(void) override;
  virtual UINT WINAPI GetCreationFlags(void) override;
  virtual HRESULT WINAPI GetDeviceRemovedReason(void) override;
  virtual void WINAPI GetImmediateContext(
    _Out_ ID3D11DeviceContext** ppImmediateContext) override;
  virtual HRESULT WINAPI SetExceptionMode(UINT RaiseFlags) override;
  virtual UINT WINAPI GetExceptionMode(void) override;

  // ID3D11Device1
  virtual void WINAPI GetImmediateContext1(
    _Out_ ID3D11DeviceContext1** ppImmediateContext) override;
  virtual HRESULT WINAPI CreateDeferredContext1(
    UINT ContextFlags,
    _Out_opt_ ID3D11DeviceContext1** ppDeferredContext) override;
  virtual HRESULT WINAPI
    CreateBlendState1(_In_ const D3D11_BLEND_DESC1* pBlendStateDesc,
                      _Out_opt_ ID3D11BlendState1** ppBlendState) override;
  virtual HRESULT WINAPI CreateRasterizerState1(
    _In_ const D3D11_RASTERIZER_DESC1* pRasterizerDesc,
    _Out_opt_ ID3D11RasterizerState1** ppRasterizerState) override;
  virtual HRESULT WINAPI CreateDeviceContextState(
    UINT Flags,
    _In_reads_(FeatureLevels) const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    REFIID EmulatedInterface,
    _Out_opt_ D3D_FEATURE_LEVEL* pChosenFeatureLevel,
    _Out_opt_ ID3DDeviceContextState** ppContextState) override;
  virtual HRESULT WINAPI OpenSharedResource1(_In_ HANDLE hResource,
                                             _In_ REFIID returnedInterface,
                                             _Out_ void** ppResource) override;
  virtual HRESULT WINAPI
    OpenSharedResourceByName(_In_ LPCWSTR lpName,
                             _In_ DWORD dwDesiredAccess,
                             _In_ REFIID returnedInterface,
                             _Out_ void** ppResource) override;

  // ID3D11Device2
  virtual void WINAPI GetImmediateContext2(
    _Out_ ID3D11DeviceContext2** ppImmediateContext) override;
  virtual HRESULT WINAPI CreateDeferredContext2(
    UINT ContextFlags,
    _Out_opt_ ID3D11DeviceContext2** ppDeferredContext) override;
  virtual void WINAPI GetResourceTiling(
    _In_ ID3D11Resource* pTiledResource,
    _Out_opt_ UINT* pNumTilesForEntireResource,
    _Out_opt_ D3D11_PACKED_MIP_DESC* pPackedMipDesc,
    _Out_opt_ D3D11_TILE_SHAPE* pStandardTileShapeForNonPackedMips,
    _Inout_opt_ UINT* pNumSubresourceTilings,
    _In_ UINT FirstSubresourceTilingToGet,
    _Out_writes_(*pNumSubresourceTilings)
      D3D11_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips) override;
  virtual HRESULT WINAPI
    CheckMultisampleQualityLevels1(_In_ DXGI_FORMAT Format,
                                   _In_ UINT SampleCount,
                                   _In_ UINT Flags,
                                   _Out_ UINT* pNumQualityLevels) override;

protected:
  void Cleanup();

  std::int64_t refs_{1};
  ID3D11Device2* device_{};
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)
}
}
