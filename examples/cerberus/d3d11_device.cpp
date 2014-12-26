// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d11_device.hpp"

#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>

#include "d3d11.hpp"

namespace hadesmem
{
namespace cerberus
{
HRESULT WINAPI D3D11DeviceProxy::QueryInterface(REFIID riid, void** obj)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  last_error_preserver.Revert();
  auto const ret = device_->QueryInterface(riid, obj);
  last_error_preserver.Update();

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    // Unknown UUIDs. Observed in Wildstar.
    UUID const unknown_uuid_1 = {0x9b7e4a00,
                                 0x342c,
                                 0x4106,
                                 0xa1,
                                 0x9f,
                                 0x4f,
                                 0x27,
                                 0x04,
                                 0xf6,
                                 0x89,
                                 0xf0};
    UUID const unknown_uuid_2 = {0xf74ee86f,
                                 0x7270,
                                 0x48e8,
                                 0x9d,
                                 0x63,
                                 0x38,
                                 0xaf,
                                 0x75,
                                 0xf2,
                                 0x2d,
                                 0x57};

    if (*obj == device_)
    {
      refs_++;
      *obj = this;
    }
#if !defined(HADESMEM_GCC)
    else if (riid == __uuidof(ID3D11Device1))
    {
      *obj = new D3D11DeviceProxy(static_cast<ID3D11Device1*>(*obj));
    }
    else if (riid == __uuidof(ID3D11Device2))
    {
      *obj = new D3D11DeviceProxy(static_cast<ID3D11Device2*>(*obj));
    }
    else if (riid == __uuidof(IDXGIDevice2) || riid == __uuidof(IDXGIDevice1) ||
             riid == __uuidof(IDXGIDevice))
    {
      // Needs investigation to see if we need to wrap this (probably do if it's
      // possible to get the 'real' ID3D11DeviceN pointer back out.
      HADESMEM_DETAIL_TRACE_A("WARNING! Potentially unhandled interface.");
      return ret;
    }
#endif // #if !defined(HADESMEM_GCC)
    else if (riid == unknown_uuid_1 || riid == unknown_uuid_2)
    {
      // Needs investigation to see if we need to wrap this.
      HADESMEM_DETAIL_TRACE_A("WARNING! Potentially unhandled interface.");
      return ret;
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

ULONG WINAPI D3D11DeviceProxy::AddRef()
{
  refs_++;
  auto const ret = device_->AddRef();
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);
  return ret;
}

ULONG WINAPI D3D11DeviceProxy::Release()
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

// ID3D11Device
HRESULT WINAPI D3D11DeviceProxy::CreateBuffer(
  _In_ const D3D11_BUFFER_DESC* pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA* pInitialData,
  _Out_opt_ ID3D11Buffer** ppBuffer)
{
  return device_->CreateBuffer(pDesc, pInitialData, ppBuffer);
}

HRESULT WINAPI D3D11DeviceProxy::CreateTexture1D(
  _In_ const D3D11_TEXTURE1D_DESC* pDesc,
  _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))
    const D3D11_SUBRESOURCE_DATA* pInitialData,
  _Out_opt_ ID3D11Texture1D** ppTexture1D)
{
  return device_->CreateTexture1D(pDesc, pInitialData, ppTexture1D);
}

HRESULT WINAPI D3D11DeviceProxy::CreateTexture2D(
  _In_ const D3D11_TEXTURE2D_DESC* pDesc,
  _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize))
    const D3D11_SUBRESOURCE_DATA* pInitialData,
  _Out_opt_ ID3D11Texture2D** ppTexture2D)
{
  return device_->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
}

HRESULT WINAPI D3D11DeviceProxy::CreateTexture3D(
  _In_ const D3D11_TEXTURE3D_DESC* pDesc,
  _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))
    const D3D11_SUBRESOURCE_DATA* pInitialData,
  _Out_opt_ ID3D11Texture3D** ppTexture3D)
{
  return device_->CreateTexture3D(pDesc, pInitialData, ppTexture3D);
}

HRESULT WINAPI D3D11DeviceProxy::CreateShaderResourceView(
  _In_ ID3D11Resource* pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
  _Out_opt_ ID3D11ShaderResourceView** ppSRView)
{
  return device_->CreateShaderResourceView(pResource, pDesc, ppSRView);
}

HRESULT WINAPI D3D11DeviceProxy::CreateUnorderedAccessView(
  _In_ ID3D11Resource* pResource,
  _In_opt_ const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
  _Out_opt_ ID3D11UnorderedAccessView** ppUAView)
{
  return device_->CreateUnorderedAccessView(pResource, pDesc, ppUAView);
}

HRESULT WINAPI D3D11DeviceProxy::CreateRenderTargetView(
  _In_ ID3D11Resource* pResource,
  _In_opt_ const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
  _Out_opt_ ID3D11RenderTargetView** ppRTView)
{
  return device_->CreateRenderTargetView(pResource, pDesc, ppRTView);
}

HRESULT WINAPI D3D11DeviceProxy::CreateDepthStencilView(
  _In_ ID3D11Resource* pResource,
  _In_opt_ const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
  _Out_opt_ ID3D11DepthStencilView** ppDepthStencilView)
{
  return device_->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView);
}

HRESULT WINAPI D3D11DeviceProxy::CreateInputLayout(
  _In_reads_(NumElements) const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
  _In_range_(0, D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT) UINT NumElements,
  _In_reads_(BytecodeLength) const void* pShaderBytecodeWithInputSignature,
  _In_ SIZE_T BytecodeLength,
  _Out_opt_ ID3D11InputLayout** ppInputLayout)
{
  return device_->CreateInputLayout(pInputElementDescs,
                                    NumElements,
                                    pShaderBytecodeWithInputSignature,
                                    BytecodeLength,
                                    ppInputLayout);
}

HRESULT WINAPI D3D11DeviceProxy::CreateVertexShader(
  _In_reads_(BytecodeLength) const void* pShaderBytecode,
  _In_ SIZE_T BytecodeLength,
  _In_opt_ ID3D11ClassLinkage* pClassLinkage,
  _Out_opt_ ID3D11VertexShader** ppVertexShader)
{
  return device_->CreateVertexShader(
    pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader);
}

HRESULT WINAPI D3D11DeviceProxy::CreateGeometryShader(
  _In_reads_(BytecodeLength) const void* pShaderBytecode,
  _In_ SIZE_T BytecodeLength,
  _In_opt_ ID3D11ClassLinkage* pClassLinkage,
  _Out_opt_ ID3D11GeometryShader** ppGeometryShader)
{
  return device_->CreateGeometryShader(
    pShaderBytecode, BytecodeLength, pClassLinkage, ppGeometryShader);
}

HRESULT WINAPI D3D11DeviceProxy::CreateGeometryShaderWithStreamOutput(
  _In_reads_(BytecodeLength) const void* pShaderBytecode,
  _In_ SIZE_T BytecodeLength,
  _In_reads_opt_(NumEntries) const D3D11_SO_DECLARATION_ENTRY* pSODeclaration,
  _In_range_(0, D3D11_SO_STREAM_COUNT* D3D11_SO_OUTPUT_COMPONENT_COUNT)
    UINT NumEntries,
  _In_reads_opt_(NumStrides) const UINT* pBufferStrides,
  _In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT) UINT NumStrides,
  _In_ UINT RasterizedStream,
  _In_opt_ ID3D11ClassLinkage* pClassLinkage,
  _Out_opt_ ID3D11GeometryShader** ppGeometryShader)
{
  return device_->CreateGeometryShaderWithStreamOutput(pShaderBytecode,
                                                       BytecodeLength,
                                                       pSODeclaration,
                                                       NumEntries,
                                                       pBufferStrides,
                                                       NumStrides,
                                                       RasterizedStream,
                                                       pClassLinkage,
                                                       ppGeometryShader);
}

HRESULT WINAPI D3D11DeviceProxy::CreatePixelShader(
  _In_reads_(BytecodeLength) const void* pShaderBytecode,
  _In_ SIZE_T BytecodeLength,
  _In_opt_ ID3D11ClassLinkage* pClassLinkage,
  _Out_opt_ ID3D11PixelShader** ppPixelShader)
{
  return device_->CreatePixelShader(
    pShaderBytecode, BytecodeLength, pClassLinkage, ppPixelShader);
}

HRESULT WINAPI
  D3D11DeviceProxy::CreateHullShader(_In_reads_(BytecodeLength)
                                       const void* pShaderBytecode,
                                     _In_ SIZE_T BytecodeLength,
                                     _In_opt_ ID3D11ClassLinkage* pClassLinkage,
                                     _Out_opt_ ID3D11HullShader** ppHullShader)
{
  return device_->CreateHullShader(
    pShaderBytecode, BytecodeLength, pClassLinkage, ppHullShader);
}

HRESULT WINAPI D3D11DeviceProxy::CreateDomainShader(
  _In_reads_(BytecodeLength) const void* pShaderBytecode,
  _In_ SIZE_T BytecodeLength,
  _In_opt_ ID3D11ClassLinkage* pClassLinkage,
  _Out_opt_ ID3D11DomainShader** ppDomainShader)
{
  return device_->CreateDomainShader(
    pShaderBytecode, BytecodeLength, pClassLinkage, ppDomainShader);
}

HRESULT WINAPI D3D11DeviceProxy::CreateComputeShader(
  _In_reads_(BytecodeLength) const void* pShaderBytecode,
  _In_ SIZE_T BytecodeLength,
  _In_opt_ ID3D11ClassLinkage* pClassLinkage,
  _Out_opt_ ID3D11ComputeShader** ppComputeShader)
{
  return device_->CreateComputeShader(
    pShaderBytecode, BytecodeLength, pClassLinkage, ppComputeShader);
}

HRESULT WINAPI
  D3D11DeviceProxy::CreateClassLinkage(_Out_ ID3D11ClassLinkage** ppLinkage)
{
  return device_->CreateClassLinkage(ppLinkage);
}

HRESULT WINAPI D3D11DeviceProxy::CreateBlendState(
  _In_ const D3D11_BLEND_DESC* pBlendStateDesc,
  _Out_opt_ ID3D11BlendState** ppBlendState)
{
  return device_->CreateBlendState(pBlendStateDesc, ppBlendState);
}

HRESULT WINAPI D3D11DeviceProxy::CreateDepthStencilState(
  _In_ const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
  _Out_opt_ ID3D11DepthStencilState** ppDepthStencilState)
{
  return device_->CreateDepthStencilState(pDepthStencilDesc,
                                          ppDepthStencilState);
}

HRESULT WINAPI D3D11DeviceProxy::CreateRasterizerState(
  _In_ const D3D11_RASTERIZER_DESC* pRasterizerDesc,
  _Out_opt_ ID3D11RasterizerState** ppRasterizerState)
{
  return device_->CreateRasterizerState(pRasterizerDesc, ppRasterizerState);
}

HRESULT WINAPI D3D11DeviceProxy::CreateSamplerState(
  _In_ const D3D11_SAMPLER_DESC* pSamplerDesc,
  _Out_opt_ ID3D11SamplerState** ppSamplerState)
{
  return device_->CreateSamplerState(pSamplerDesc, ppSamplerState);
}

HRESULT WINAPI
  D3D11DeviceProxy::CreateQuery(_In_ const D3D11_QUERY_DESC* pQueryDesc,
                                _Out_opt_ ID3D11Query** ppQuery)
{
  return device_->CreateQuery(pQueryDesc, ppQuery);
}

HRESULT WINAPI
  D3D11DeviceProxy::CreatePredicate(_In_ const D3D11_QUERY_DESC* pPredicateDesc,
                                    _Out_opt_ ID3D11Predicate** ppPredicate)
{
  return device_->CreatePredicate(pPredicateDesc, ppPredicate);
}

HRESULT WINAPI
  D3D11DeviceProxy::CreateCounter(_In_ const D3D11_COUNTER_DESC* pCounterDesc,
                                  _Out_opt_ ID3D11Counter** ppCounter)
{
  return device_->CreateCounter(pCounterDesc, ppCounter);
}

HRESULT WINAPI D3D11DeviceProxy::CreateDeferredContext(
  UINT ContextFlags, _Out_opt_ ID3D11DeviceContext** ppDeferredContext)
{
  return device_->CreateDeferredContext(ContextFlags, ppDeferredContext);
}

HRESULT WINAPI
  D3D11DeviceProxy::OpenSharedResource(_In_ HANDLE hResource,
                                       _In_ REFIID ReturnedInterface,
                                       _Out_opt_ void** ppResource)
{
  return device_->OpenSharedResource(hResource, ReturnedInterface, ppResource);
}

HRESULT WINAPI D3D11DeviceProxy::CheckFormatSupport(_In_ DXGI_FORMAT Format,
                                                    _Out_ UINT* pFormatSupport)
{
  return device_->CheckFormatSupport(Format, pFormatSupport);
}

HRESULT WINAPI
  D3D11DeviceProxy::CheckMultisampleQualityLevels(_In_ DXGI_FORMAT Format,
                                                  _In_ UINT SampleCount,
                                                  _Out_ UINT* pNumQualityLevels)
{
  return device_->CheckMultisampleQualityLevels(
    Format, SampleCount, pNumQualityLevels);
}

void WINAPI
  D3D11DeviceProxy::CheckCounterInfo(_Out_ D3D11_COUNTER_INFO* pCounterInfo)
{
  return device_->CheckCounterInfo(pCounterInfo);
}

HRESULT WINAPI
  D3D11DeviceProxy::CheckCounter(_In_ const D3D11_COUNTER_DESC* pDesc,
                                 _Out_ D3D11_COUNTER_TYPE* pType,
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

HRESULT WINAPI D3D11DeviceProxy::CheckFeatureSupport(
  D3D11_FEATURE Feature,
  _Out_writes_bytes_(FeatureSupportDataSize) void* pFeatureSupportData,
  UINT FeatureSupportDataSize)
{
  return device_->CheckFeatureSupport(
    Feature, pFeatureSupportData, FeatureSupportDataSize);
}

HRESULT WINAPI D3D11DeviceProxy::GetPrivateData(
  _In_ REFGUID guid,
  _Inout_ UINT* pDataSize,
  _Out_writes_bytes_opt_(*pDataSize) void* pData)
{
  return device_->GetPrivateData(guid, pDataSize, pData);
}

HRESULT WINAPI D3D11DeviceProxy::SetPrivateData(_In_ REFGUID guid,
                                                _In_ UINT DataSize,
                                                _In_reads_bytes_opt_(DataSize)
                                                  const void* pData)
{
  return device_->SetPrivateData(guid, DataSize, pData);
}

HRESULT WINAPI
  D3D11DeviceProxy::SetPrivateDataInterface(_In_ REFGUID guid,
                                            _In_opt_ const IUnknown* pData)
{
  return device_->SetPrivateDataInterface(guid, pData);
}

D3D_FEATURE_LEVEL WINAPI D3D11DeviceProxy::GetFeatureLevel(void)
{
  return device_->GetFeatureLevel();
}

UINT WINAPI D3D11DeviceProxy::GetCreationFlags(void)
{
  return device_->GetCreationFlags();
}

HRESULT WINAPI D3D11DeviceProxy::GetDeviceRemovedReason(void)
{
  return device_->GetDeviceRemovedReason();
}

void WINAPI D3D11DeviceProxy::GetImmediateContext(
  _Out_ ID3D11DeviceContext** ppImmediateContext)
{
  return device_->GetImmediateContext(ppImmediateContext);
}

HRESULT WINAPI D3D11DeviceProxy::SetExceptionMode(UINT RaiseFlags)
{
  return device_->SetExceptionMode(RaiseFlags);
}

UINT WINAPI D3D11DeviceProxy::GetExceptionMode(void)
{
  return device_->GetExceptionMode();
}

#if !defined(HADESMEM_GCC)
// ID3D11Device1
void WINAPI D3D11DeviceProxy::GetImmediateContext1(
  _Out_ ID3D11DeviceContext1** ppImmediateContext)
{
  auto const device = static_cast<ID3D11Device1*>(device_);
  return device->GetImmediateContext1(ppImmediateContext);
}

HRESULT WINAPI D3D11DeviceProxy::CreateDeferredContext1(
  UINT ContextFlags, _Out_opt_ ID3D11DeviceContext1** ppDeferredContext)
{
  auto const device = static_cast<ID3D11Device1*>(device_);
  return device->CreateDeferredContext1(ContextFlags, ppDeferredContext);
}

HRESULT WINAPI D3D11DeviceProxy::CreateBlendState1(
  _In_ const D3D11_BLEND_DESC1* pBlendStateDesc,
  _Out_opt_ ID3D11BlendState1** ppBlendState)
{
  auto const device = static_cast<ID3D11Device1*>(device_);
  return device->CreateBlendState1(pBlendStateDesc, ppBlendState);
}

HRESULT WINAPI D3D11DeviceProxy::CreateRasterizerState1(
  _In_ const D3D11_RASTERIZER_DESC1* pRasterizerDesc,
  _Out_opt_ ID3D11RasterizerState1** ppRasterizerState)
{
  auto const device = static_cast<ID3D11Device1*>(device_);
  return device->CreateRasterizerState1(pRasterizerDesc, ppRasterizerState);
}

HRESULT WINAPI D3D11DeviceProxy::CreateDeviceContextState(
  UINT Flags,
  _In_reads_(FeatureLevels) const D3D_FEATURE_LEVEL* pFeatureLevels,
  UINT FeatureLevels,
  UINT SDKVersion,
  REFIID EmulatedInterface,
  _Out_opt_ D3D_FEATURE_LEVEL* pChosenFeatureLevel,
  _Out_opt_ ID3DDeviceContextState** ppContextState)
{
  auto const device = static_cast<ID3D11Device1*>(device_);
  return device->CreateDeviceContextState(Flags,
                                          pFeatureLevels,
                                          FeatureLevels,
                                          SDKVersion,
                                          EmulatedInterface,
                                          pChosenFeatureLevel,
                                          ppContextState);
}

HRESULT WINAPI
  D3D11DeviceProxy::OpenSharedResource1(_In_ HANDLE hResource,
                                        _In_ REFIID returnedInterface,
                                        _Out_ void** ppResource)
{
  auto const device = static_cast<ID3D11Device1*>(device_);
  return device->OpenSharedResource1(hResource, returnedInterface, ppResource);
}

HRESULT WINAPI
  D3D11DeviceProxy::OpenSharedResourceByName(_In_ LPCWSTR lpName,
                                             _In_ DWORD dwDesiredAccess,
                                             _In_ REFIID returnedInterface,
                                             _Out_ void** ppResource)
{
  auto const device = static_cast<ID3D11Device1*>(device_);
  return device->OpenSharedResourceByName(
    lpName, dwDesiredAccess, returnedInterface, ppResource);
}

// ID3D11Device2
void WINAPI D3D11DeviceProxy::GetImmediateContext2(
  _Out_ ID3D11DeviceContext2** ppImmediateContext)
{
  auto const device = static_cast<ID3D11Device2*>(device_);
  return device->GetImmediateContext2(ppImmediateContext);
}

HRESULT WINAPI D3D11DeviceProxy::CreateDeferredContext2(
  UINT ContextFlags, _Out_opt_ ID3D11DeviceContext2** ppDeferredContext)
{
  auto const device = static_cast<ID3D11Device2*>(device_);
  return device->CreateDeferredContext2(ContextFlags, ppDeferredContext);
}

void WINAPI D3D11DeviceProxy::GetResourceTiling(
  _In_ ID3D11Resource* pTiledResource,
  _Out_opt_ UINT* pNumTilesForEntireResource,
  _Out_opt_ D3D11_PACKED_MIP_DESC* pPackedMipDesc,
  _Out_opt_ D3D11_TILE_SHAPE* pStandardTileShapeForNonPackedMips,
  _Inout_opt_ UINT* pNumSubresourceTilings,
  _In_ UINT FirstSubresourceTilingToGet,
  _Out_writes_(*pNumSubresourceTilings)
    D3D11_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips)
{
  auto const device = static_cast<ID3D11Device2*>(device_);
  return device->GetResourceTiling(pTiledResource,
                                   pNumTilesForEntireResource,
                                   pPackedMipDesc,
                                   pStandardTileShapeForNonPackedMips,
                                   pNumSubresourceTilings,
                                   FirstSubresourceTilingToGet,
                                   pSubresourceTilingsForNonPackedMips);
}

HRESULT WINAPI D3D11DeviceProxy::CheckMultisampleQualityLevels1(
  _In_ DXGI_FORMAT Format,
  _In_ UINT SampleCount,
  _In_ UINT Flags,
  _Out_ UINT* pNumQualityLevels)
{
  auto const device = static_cast<ID3D11Device2*>(device_);
  return device->CheckMultisampleQualityLevels1(
    Format, SampleCount, Flags, pNumQualityLevels);
}
#endif // #if !defined(HADESMEM_GCC)

void D3D11DeviceProxy::Cleanup()
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  auto& callbacks = GetOnReleaseD3D11Callbacks();
  callbacks.Run(device_);
}
}
}
