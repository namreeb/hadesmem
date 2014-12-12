// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <cstring>

#include <d3d11.h>

#include "d3d11_defs.hpp"

namespace hadesmem
{
namespace cerberus
{
class D3D11StateBlock
{
public:
  D3D11StateBlock(ID3D11DeviceContext* context)
    : context_{context}, state_block_{}
  {
    Capture();
  }

  ~D3D11StateBlock()
  {
    Free();
  }

  void Capture()
  {
    Free();

    Capture(context_, &state_block_);
  }

  void Apply()
  {
    Apply(context_, &state_block_);
  }

private:
  struct D3D11_STATE_BLOCK
  {
    ID3D11VertexShader* VS;
    ID3D11SamplerState* VSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D11ShaderResourceView*
      VSShaderResources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11Buffer*
      VSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ClassInstance* VSInterfaces[D3D11_SHADER_MAX_INTERFACES];
    UINT VSInterfaceCount;

    ID3D11GeometryShader* GS;
    ID3D11SamplerState* GSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D11ShaderResourceView*
      GSShaderResources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11Buffer*
      GSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ClassInstance* GSInterfaces[D3D11_SHADER_MAX_INTERFACES];
    UINT GSInterfaceCount;

    ID3D11HullShader* HS;
    ID3D11SamplerState* HSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D11ShaderResourceView*
      HSShaderResources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11Buffer*
      HSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ClassInstance* HSInterfaces[D3D11_SHADER_MAX_INTERFACES];
    UINT HSInterfaceCount;

    ID3D11DomainShader* DS;
    ID3D11SamplerState* DSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D11ShaderResourceView*
      DSShaderResources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11Buffer*
      DSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ClassInstance* DSInterfaces[D3D11_SHADER_MAX_INTERFACES];
    UINT DSInterfaceCount;

    ID3D11PixelShader* PS;
    ID3D11SamplerState* PSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D11ShaderResourceView*
      PSShaderResources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11Buffer*
      PSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ClassInstance* PSInterfaces[D3D11_SHADER_MAX_INTERFACES];
    UINT PSInterfaceCount;

    ID3D11ComputeShader* CS;
    ID3D11SamplerState* CSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    ID3D11ShaderResourceView*
      CSShaderResources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11Buffer*
      CSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    ID3D11ClassInstance* CSInterfaces[D3D11_SHADER_MAX_INTERFACES];
    UINT CSInterfaceCount;
    ID3D11UnorderedAccessView*
      CSUnorderedAccessViews[D3D11_PS_CS_UAV_REGISTER_COUNT];

    ID3D11Buffer* IAVertexBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    UINT IAVertexBuffersStrides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    UINT IAVertexBuffersOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11Buffer* IAIndexBuffer;
    DXGI_FORMAT IAIndexBufferFormat;
    UINT IAIndexBufferOffset;
    ID3D11InputLayout* IAInputLayout;
    D3D11_PRIMITIVE_TOPOLOGY IAPrimitiveTopology;

    ID3D11RenderTargetView*
      OMRenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D11DepthStencilView* OMRenderTargetStencilView;
    ID3D11UnorderedAccessView*
      OMUnorderedAccessViews[D3D11_PS_CS_UAV_REGISTER_COUNT];
    ID3D11DepthStencilState* OMDepthStencilState;
    UINT OMDepthStencilRef;
    ID3D11BlendState* OMBlendState;
    FLOAT OMBlendFactor[4];
    UINT OMSampleMask;

    UINT RSViewportCount;
    D3D11_VIEWPORT
    RSViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    UINT RSScissorRectCount;
    D3D11_RECT
    RSScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D11RasterizerState* RSRasterizerState;
    ID3D11Buffer* SOBuffers[4];
    ID3D11Predicate* Predication;
    BOOL PredicationValue;
  };

  template <typename T> void SafeRelease(T* p)
  {
    if (p)
    {
      p->Release();
    }
  }

  template <typename T, std::size_t N> void SafeReleaseArray(T(&a)[N])
  {
    for (auto const& p : a)
    {
      SafeRelease(p);
    }
  }

  void Free()
  {
    SafeRelease(state_block_.VS);
    SafeReleaseArray(state_block_.VSSamplers);
    SafeReleaseArray(state_block_.VSShaderResources);
    SafeReleaseArray(state_block_.VSConstantBuffers);
    SafeReleaseArray(state_block_.VSInterfaces);

    SafeRelease(state_block_.GS);
    SafeReleaseArray(state_block_.GSSamplers);
    SafeReleaseArray(state_block_.GSShaderResources);
    SafeReleaseArray(state_block_.GSConstantBuffers);
    SafeReleaseArray(state_block_.GSInterfaces);

    SafeRelease(state_block_.HS);
    SafeReleaseArray(state_block_.HSSamplers);
    SafeReleaseArray(state_block_.HSShaderResources);
    SafeReleaseArray(state_block_.HSConstantBuffers);
    SafeReleaseArray(state_block_.HSInterfaces);

    SafeRelease(state_block_.DS);
    SafeReleaseArray(state_block_.DSSamplers);
    SafeReleaseArray(state_block_.DSShaderResources);
    SafeReleaseArray(state_block_.DSConstantBuffers);
    SafeReleaseArray(state_block_.DSInterfaces);

    SafeRelease(state_block_.PS);
    SafeReleaseArray(state_block_.PSSamplers);
    SafeReleaseArray(state_block_.PSShaderResources);
    SafeReleaseArray(state_block_.PSConstantBuffers);
    SafeReleaseArray(state_block_.PSInterfaces);

    SafeRelease(state_block_.CS);
    SafeReleaseArray(state_block_.CSSamplers);
    SafeReleaseArray(state_block_.CSShaderResources);
    SafeReleaseArray(state_block_.CSConstantBuffers);
    SafeReleaseArray(state_block_.CSInterfaces);
    SafeReleaseArray(state_block_.CSUnorderedAccessViews);

    SafeReleaseArray(state_block_.IAVertexBuffers);

    SafeRelease(state_block_.IAIndexBuffer);
    SafeRelease(state_block_.IAInputLayout);

    SafeReleaseArray(state_block_.OMRenderTargets);

    SafeRelease(state_block_.OMRenderTargetStencilView);

    SafeReleaseArray(state_block_.OMUnorderedAccessViews);

    SafeRelease(state_block_.OMDepthStencilState);
    SafeRelease(state_block_.OMBlendState);
    SafeRelease(state_block_.RSRasterizerState);

    SafeReleaseArray(state_block_.SOBuffers);

    SafeRelease(state_block_.Predication);
  }

  template <class T> UINT Count(T** arr, UINT max_count)
  {
    for (size_t i = 0; i < max_count; ++i)
    {
      if (arr[i] == 0)
      {
        return static_cast<UINT>(i);
      }
    }

    return max_count;
  }

  void Capture(ID3D11DeviceContext* dc, D3D11_STATE_BLOCK* sb)
  {
    std::memset(sb, 0, sizeof(D3D11_STATE_BLOCK));

    dc->VSGetShader(&sb->VS, sb->VSInterfaces, &sb->VSInterfaceCount);
    dc->VSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, sb->VSSamplers);
    dc->VSGetShaderResources(
      0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, sb->VSShaderResources);
    dc->VSGetConstantBuffers(0,
                             D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                             sb->VSConstantBuffers);

    dc->GSGetShader(&sb->GS, sb->GSInterfaces, &sb->GSInterfaceCount);
    dc->GSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, sb->GSSamplers);
    dc->GSGetShaderResources(
      0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, sb->GSShaderResources);
    dc->GSGetConstantBuffers(0,
                             D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                             sb->GSConstantBuffers);

    dc->HSGetShader(&sb->HS, sb->HSInterfaces, &sb->HSInterfaceCount);
    dc->HSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, sb->HSSamplers);
    dc->HSGetShaderResources(
      0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, sb->HSShaderResources);
    dc->HSGetConstantBuffers(0,
                             D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                             sb->HSConstantBuffers);

    dc->DSGetShader(&sb->DS, sb->DSInterfaces, &sb->DSInterfaceCount);
    dc->DSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, sb->DSSamplers);
    dc->DSGetShaderResources(
      0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, sb->DSShaderResources);
    dc->DSGetConstantBuffers(0,
                             D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                             sb->DSConstantBuffers);

    dc->PSGetShader(&sb->PS, sb->PSInterfaces, &sb->PSInterfaceCount);
    dc->PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, sb->PSSamplers);
    dc->PSGetShaderResources(
      0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, sb->PSShaderResources);
    dc->PSGetConstantBuffers(0,
                             D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                             sb->PSConstantBuffers);

    dc->CSGetShader(&sb->CS, sb->CSInterfaces, &sb->CSInterfaceCount);
    dc->CSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, sb->CSSamplers);
    dc->CSGetShaderResources(
      0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, sb->CSShaderResources);
    dc->CSGetConstantBuffers(0,
                             D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                             sb->CSConstantBuffers);
    dc->CSGetUnorderedAccessViews(
      0, D3D11_PS_CS_UAV_REGISTER_COUNT, sb->CSUnorderedAccessViews);

    dc->IAGetVertexBuffers(0,
                           D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
                           sb->IAVertexBuffers,
                           sb->IAVertexBuffersStrides,
                           sb->IAVertexBuffersOffsets);
    dc->IAGetIndexBuffer(
      &sb->IAIndexBuffer, &sb->IAIndexBufferFormat, &sb->IAIndexBufferOffset);
    dc->IAGetInputLayout(&sb->IAInputLayout);
    dc->IAGetPrimitiveTopology(&sb->IAPrimitiveTopology);

    dc->OMGetRenderTargetsAndUnorderedAccessViews(
      D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,
      sb->OMRenderTargets,
      &sb->OMRenderTargetStencilView,
      0,
      D3D11_PS_CS_UAV_REGISTER_COUNT,
      sb->OMUnorderedAccessViews);
    dc->OMGetDepthStencilState(&sb->OMDepthStencilState,
                               &sb->OMDepthStencilRef);
    dc->OMGetBlendState(
      &sb->OMBlendState, sb->OMBlendFactor, &sb->OMSampleMask);

    sb->RSViewportCount =
      D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    dc->RSGetViewports(&sb->RSViewportCount, sb->RSViewports);
    sb->RSScissorRectCount =
      D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    dc->RSGetScissorRects(&sb->RSScissorRectCount, sb->RSScissorRects);
    dc->RSGetState(&sb->RSRasterizerState);

    dc->SOGetTargets(4, sb->SOBuffers);
    dc->GetPredication(&sb->Predication, &sb->PredicationValue);
  }

  void Apply(ID3D11DeviceContext* dc, D3D11_STATE_BLOCK* sb)
  {
    UINT minus_one[D3D11_PS_CS_UAV_REGISTER_COUNT];
    std::memset(minus_one, -1, sizeof(minus_one));
    dc->VSSetShader(sb->VS, sb->VSInterfaces, sb->VSInterfaceCount);
    if (UINT const VSSamplerCount =
          Count(sb->VSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT))
    {
      dc->VSSetSamplers(0, VSSamplerCount, sb->VSSamplers);
    }
    if (UINT const VSShaderResourceCount = Count(
          sb->VSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT))
    {
      dc->VSSetShaderResources(0, VSShaderResourceCount, sb->VSShaderResources);
    }
    if (UINT const VSConstantBufferCount =
          Count(sb->VSConstantBuffers,
                D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT))
    {
      dc->VSSetConstantBuffers(0, VSConstantBufferCount, sb->VSConstantBuffers);
    }

    dc->GSSetShader(sb->GS, sb->GSInterfaces, sb->GSInterfaceCount);
    if (UINT const GSSamplerCount =
          Count(sb->GSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT))
    {
      dc->GSSetSamplers(0, GSSamplerCount, sb->GSSamplers);
    }
    if (UINT const GSShaderResourceCount = Count(
          sb->GSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT))
    {
      dc->GSSetShaderResources(0, GSShaderResourceCount, sb->GSShaderResources);
    }
    if (UINT const GSConstantBufferCount =
          Count(sb->GSConstantBuffers,
                D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT))
    {
      dc->GSSetConstantBuffers(0, GSConstantBufferCount, sb->GSConstantBuffers);
    }

    dc->HSSetShader(sb->HS, sb->HSInterfaces, sb->HSInterfaceCount);
    if (UINT const HSSamplerCount =
          Count(sb->HSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT))
    {
      dc->HSSetSamplers(0, HSSamplerCount, sb->HSSamplers);
    }
    if (UINT const HSShaderResourceCount = Count(
          sb->HSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT))
    {
      dc->HSSetShaderResources(0, HSShaderResourceCount, sb->HSShaderResources);
    }
    if (UINT const HSConstantBufferCount =
          Count(sb->HSConstantBuffers,
                D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT))
    {
      dc->HSSetConstantBuffers(0, HSConstantBufferCount, sb->HSConstantBuffers);
    }

    dc->DSSetShader(sb->DS, sb->DSInterfaces, sb->DSInterfaceCount);
    if (UINT const DSSamplerCount =
          Count(sb->DSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT))
    {
      dc->DSSetSamplers(0, DSSamplerCount, sb->DSSamplers);
    }
    if (UINT const DSShaderResourceCount = Count(
          sb->DSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT))
    {
      dc->DSSetShaderResources(0, DSShaderResourceCount, sb->DSShaderResources);
    }
    if (UINT const DSConstantBufferCount =
          Count(sb->DSConstantBuffers,
                D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT))
    {
      dc->DSSetConstantBuffers(0, DSConstantBufferCount, sb->DSConstantBuffers);
    }

    dc->PSSetShader(sb->PS, sb->PSInterfaces, sb->PSInterfaceCount);
    if (UINT const PSSamplerCount =
          Count(sb->PSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT))
    {
      dc->PSSetSamplers(0, PSSamplerCount, sb->PSSamplers);
    }
    if (UINT const PSShaderResourceCount = Count(
          sb->PSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT))
    {
      dc->PSSetShaderResources(0, PSShaderResourceCount, sb->PSShaderResources);
    }
    if (UINT const PSConstantBufferCount =
          Count(sb->PSConstantBuffers,
                D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT))
    {
      dc->PSSetConstantBuffers(0, PSConstantBufferCount, sb->PSConstantBuffers);
    }

    dc->CSSetShader(sb->CS, sb->CSInterfaces, sb->CSInterfaceCount);
    if (UINT const CSSamplerCount =
          Count(sb->CSSamplers, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT))
    {
      dc->CSSetSamplers(0, CSSamplerCount, sb->CSSamplers);
    }
    if (UINT const CSShaderResourceCount = Count(
          sb->CSShaderResources, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT))
    {
      dc->CSSetShaderResources(0, CSShaderResourceCount, sb->CSShaderResources);
    }
    if (UINT const CSConstantBufferCount =
          Count(sb->CSConstantBuffers,
                D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT))
    {
      dc->CSSetConstantBuffers(0, CSConstantBufferCount, sb->CSConstantBuffers);
    }
    // dc->CSSetUnorderedAccessViews(0, D3D11_PS_CS_UAV_REGISTER_COUNT,
    // sb->CSUnorderedAccessViews, minus_one);

    if (UINT const IAVertexBufferCount =
          Count(sb->IAVertexBuffers, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT))
    {
      dc->IASetVertexBuffers(0,
                             IAVertexBufferCount,
                             sb->IAVertexBuffers,
                             sb->IAVertexBuffersStrides,
                             sb->IAVertexBuffersOffsets);
    }
    dc->IASetIndexBuffer(
      sb->IAIndexBuffer, sb->IAIndexBufferFormat, sb->IAIndexBufferOffset);
    dc->IASetInputLayout(sb->IAInputLayout);
    dc->IASetPrimitiveTopology(sb->IAPrimitiveTopology);

    // dc->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,
    // sb->OMRenderTargets, sb->OMRenderTargetStencilView, 0,
    // D3D11_PS_CS_UAV_REGISTER_COUNT, sb->OMUnorderedAccessViews, minus_one);
    if (UINT const OMRenderTargetCount =
          Count(sb->OMRenderTargets, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT))
    {
      dc->OMSetRenderTargets(OMRenderTargetCount,
                             sb->OMRenderTargets,
                             sb->OMRenderTargetStencilView);
    }
    dc->OMSetDepthStencilState(sb->OMDepthStencilState, sb->OMDepthStencilRef);
    dc->OMSetBlendState(sb->OMBlendState, sb->OMBlendFactor, sb->OMSampleMask);

    dc->RSSetViewports(sb->RSViewportCount, sb->RSViewports);
    dc->RSSetScissorRects(sb->RSScissorRectCount, sb->RSScissorRects);
    dc->RSSetState(sb->RSRasterizerState);

    if (UINT const SOBufferCount = Count(sb->SOBuffers, 4))
    {
      UINT const SOBuffersOffsets[4] = {0};
      dc->SOSetTargets(SOBufferCount, sb->SOBuffers, SOBuffersOffsets);
    }

    dc->SetPredication(sb->Predication, sb->PredicationValue);
  }

  ID3D11DeviceContext* context_;
  D3D11_STATE_BLOCK state_block_;
};
}
}
