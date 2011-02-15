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
#include "HadesCommon/Error.hpp"
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace D3D9
  {
    IDirect3DDevice9Hook::IDirect3DDevice9Hook(
      Kernel::Kernel* pKernel, 
      IDirect3D9* pD3D9, 
      IDirect3DDevice9* pDevice, 
      D3DPRESENT_PARAMETERS* pPresentParams) 
      : m_pKernel(pKernel), 
      m_pD3D(pD3D9), 
      m_pDevice(pDevice), 
      m_PresentParams(), 
      m_RefCount(1), 
      m_pRenderer()
    {
      if (!pD3D9)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("IDirect3DDevice9Hook::IDirect3DDevice9Hook") << 
          ErrorString("Invalid interface pointer.") << 
          ErrorCode(LastError));
      }
      
      if (!pDevice)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("IDirect3DDevice9Hook::IDirect3DDevice9Hook") << 
          ErrorString("Invalid device pointer.") << 
          ErrorCode(LastError));
      }
      
      if (!pPresentParams)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("IDirect3DDevice9Hook::IDirect3DDevice9Hook") << 
          ErrorString("Invalid presentation params.") << 
          ErrorCode(LastError));
      }
      
      m_PresentParams = *pPresentParams;
      
      m_pRenderer.reset(new GUI::D3D9Renderer(m_pDevice));
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::QueryInterface(
      REFIID riid, 
      LPVOID *ppvObj) 
    {
      return m_pDevice->QueryInterface(
        riid, 
        ppvObj);
    }
    
    ULONG APIENTRY IDirect3DDevice9Hook::AddRef() 
    {
      m_RefCount++;
      return m_pDevice->AddRef();
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::BeginScene() 
    {
      return m_pDevice->BeginScene();
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::BeginStateBlock() 
    {
      return m_pDevice->BeginStateBlock();
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::Clear(DWORD Count, 
      CONST D3DRECT *pRects, 
      DWORD Flags, 
      D3DCOLOR Color, 
      float Z, 
      DWORD Stencil) 
    {
      return m_pDevice->Clear(
        Count, 
        pRects, 
        Flags, 
        Color, 
        Z, 
        Stencil);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::ColorFill(
      IDirect3DSurface9* pSurface, 
      CONST RECT* pRect, 
      D3DCOLOR color) 
    { 
      return m_pDevice->ColorFill(
      pSurface, 
      pRect, 
      color);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateAdditionalSwapChain(
      D3DPRESENT_PARAMETERS *pPresentationParameters, 
      IDirect3DSwapChain9 **ppSwapChain) 
    {
      return m_pDevice->CreateAdditionalSwapChain(
        pPresentationParameters, 
        ppSwapChain);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateCubeTexture(
        UINT EdgeLength, 
        UINT Levels, 
        DWORD Usage, 
        D3DFORMAT Format, 
        D3DPOOL Pool, 
        IDirect3DCubeTexture9** ppCubeTexture, 
        HANDLE* pSharedHandle) 
    {
      return m_pDevice->CreateCubeTexture(
        EdgeLength, 
        Levels, 
        Usage, 
        Format, 
        Pool, 
        ppCubeTexture, 
        pSharedHandle);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateDepthStencilSurface(
      UINT Width, 
      UINT Height, 
      D3DFORMAT Format, 
      D3DMULTISAMPLE_TYPE MultiSample, 
      DWORD MultisampleQuality, 
      BOOL Discard, 
      IDirect3DSurface9** ppSurface, 
      HANDLE* pSharedHandle) 
    {
      return m_pDevice->CreateDepthStencilSurface(
        Width, 
        Height, 
        Format, 
        MultiSample, 
        MultisampleQuality, 
        Discard, 
        ppSurface, 
        pSharedHandle);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateIndexBuffer(
      UINT Length, 
      DWORD Usage, 
      D3DFORMAT Format, 
      D3DPOOL Pool, 
      IDirect3DIndexBuffer9** ppIndexBuffer, 
      HANDLE* pSharedHandle) 
    {
      return m_pDevice->CreateIndexBuffer(
        Length, 
        Usage, 
        Format, 
        Pool, 
        ppIndexBuffer, 
        pSharedHandle);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateOffscreenPlainSurface(
      UINT Width, 
      UINT Height, 
      D3DFORMAT Format, 
      D3DPOOL Pool, 
      IDirect3DSurface9** ppSurface, 
      HANDLE* pSharedHandle) 
    {
      return m_pDevice->CreateOffscreenPlainSurface(
        Width, 
        Height, 
        Format, 
        Pool, 
        ppSurface, 
        pSharedHandle);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreatePixelShader(
        CONST DWORD* pFunction, 
        IDirect3DPixelShader9** ppShader) 
    {
      return m_pDevice->CreatePixelShader(
        pFunction, 
        ppShader);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateQuery(
      D3DQUERYTYPE Type, 
      IDirect3DQuery9** ppQuery) 
    {
      return m_pDevice->CreateQuery(
        Type, 
        ppQuery);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateRenderTarget(
      UINT Width, 
      UINT Height, 
      D3DFORMAT Format, 
      D3DMULTISAMPLE_TYPE MultiSample, 
      DWORD MultisampleQuality, 
      BOOL Lockable, 
      IDirect3DSurface9** ppSurface, 
      HANDLE* pSharedHandle) 
    {
      return m_pDevice->CreateRenderTarget(
        Width, 
        Height, 
        Format, 
        MultiSample, 
        MultisampleQuality, 
        Lockable, 
        ppSurface, 
        pSharedHandle);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateStateBlock(
        D3DSTATEBLOCKTYPE Type, 
        IDirect3DStateBlock9** ppSB) 
    {
      return m_pDevice->CreateStateBlock(
        Type, 
        ppSB);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateTexture(
      UINT Width, 
      UINT Height, 
      UINT Levels, 
      DWORD Usage, 
      D3DFORMAT Format, 
      D3DPOOL Pool, 
      IDirect3DTexture9** ppTexture, 
      HANDLE* pSharedHandle) 
    {
      HRESULT Result = m_pDevice->CreateTexture(
        Width, 
        Height, 
        Levels, 
        Usage, 
        Format, 
        Pool, 
        ppTexture, 
        pSharedHandle);
    
      //if(ret == D3D_OK) 
      //{ 
      //  new hkIDirect3DTexture9(ppTexture, this, Width, Height, Format);
      //}
      
      return Result;
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateVertexBuffer(
      UINT Length, 
      DWORD Usage, 
      DWORD FVF, 
      D3DPOOL Pool, 
      IDirect3DVertexBuffer9** ppVertexBuffer, 
      HANDLE* pSharedHandle) 
    {
      return m_pDevice->CreateVertexBuffer(
        Length, 
        Usage, 
        FVF, 
        Pool, 
        ppVertexBuffer, 
        pSharedHandle);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateVertexDeclaration(
      CONST D3DVERTEXELEMENT9* pVertexElements, 
      IDirect3DVertexDeclaration9** ppDecl) 
    {
      return m_pDevice->CreateVertexDeclaration(
        pVertexElements, 
        ppDecl);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateVertexShader(
      CONST DWORD* pFunction, 
      IDirect3DVertexShader9** ppShader) 
    {
      return m_pDevice->CreateVertexShader(
        pFunction, 
        ppShader);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::CreateVolumeTexture(
      UINT Width, 
      UINT Height, 
      UINT Depth, 
      UINT Levels, 
      DWORD Usage, 
      D3DFORMAT Format, 
      D3DPOOL Pool, 
      IDirect3DVolumeTexture9** ppVolumeTexture, 
      HANDLE* pSharedHandle) 
    {
      return m_pDevice->CreateVolumeTexture(
        Width, 
        Height, 
        Depth, 
        Levels, 
        Usage, 
        Format, 
        Pool, 
        ppVolumeTexture, 
        pSharedHandle);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::DeletePatch(
      UINT Handle) 
    {
      return m_pDevice->DeletePatch(
        Handle);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::DrawIndexedPrimitive(
      D3DPRIMITIVETYPE Type, 
      INT BaseVertexIndex, 
      UINT MinVertexIndex, 
      UINT NumVertices, 
      UINT startIndex, 
      UINT primCount)
    {
      return m_pDevice->DrawIndexedPrimitive(
        Type, 
        BaseVertexIndex, 
        MinVertexIndex, 
        NumVertices, 
        startIndex, 
        primCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::DrawIndexedPrimitiveUP(
      D3DPRIMITIVETYPE PrimitiveType, 
      UINT MinIndex, 
      UINT NumVertices, 
      UINT PrimitiveCount, 
      CONST void *pIndexData, 
      D3DFORMAT IndexDataFormat, 
      CONST void *pVertexStreamZeroData, 
      UINT VertexStreamZeroStride) 
    { 
      return m_pDevice->DrawIndexedPrimitiveUP(
        PrimitiveType, 
        MinIndex, 
        NumVertices, 
        PrimitiveCount, 
        pIndexData, 
        IndexDataFormat, 
        pVertexStreamZeroData, 
        VertexStreamZeroStride);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::DrawPrimitive(
      D3DPRIMITIVETYPE PrimitiveType, 
      UINT StartVertex, 
      UINT PrimitiveCount) 
    {
      return m_pDevice->DrawPrimitive(
        PrimitiveType, 
        StartVertex, 
        PrimitiveCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::DrawPrimitiveUP(
      D3DPRIMITIVETYPE PrimitiveType, 
      UINT PrimitiveCount, 
      CONST void *pVertexStreamZeroData, 
      UINT VertexStreamZeroStride) 
    {
      return m_pDevice->DrawPrimitiveUP(
        PrimitiveType, 
        PrimitiveCount, 
        pVertexStreamZeroData, 
        VertexStreamZeroStride);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::DrawRectPatch(
      UINT Handle, 
      CONST float *pNumSegs, 
      CONST D3DRECTPATCH_INFO *pRectPatchInfo) 
    {
      return m_pDevice->DrawRectPatch(
        Handle, 
        pNumSegs, 
        pRectPatchInfo);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::DrawTriPatch(
      UINT Handle, 
      CONST float *pNumSegs, 
      CONST D3DTRIPATCH_INFO *pTriPatchInfo)
    {
      return m_pDevice->DrawTriPatch(
        Handle, 
        pNumSegs, 
        pTriPatchInfo);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::EndScene()
    {
      m_pKernel->OnFrame(*m_pRenderer);
      
      return m_pDevice->EndScene();
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::EndStateBlock(
      IDirect3DStateBlock9** ppSB) 
    {
      return m_pDevice->EndStateBlock(
        ppSB);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::EvictManagedResources() 
    {
      return m_pDevice->EvictManagedResources();
    }
    
    UINT APIENTRY IDirect3DDevice9Hook::GetAvailableTextureMem() 
    {
      return m_pDevice->GetAvailableTextureMem();
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetBackBuffer(
      UINT iSwapChain, 
      UINT iBackBuffer, 
      D3DBACKBUFFER_TYPE Type, 
      IDirect3DSurface9** ppBackBuffer) 
    {
      return m_pDevice->GetBackBuffer(
        iSwapChain, 
        iBackBuffer, 
        Type, 
        ppBackBuffer);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetClipPlane(
      DWORD Index, 
      float *pPlane) 
    {
      return m_pDevice->GetClipPlane(
        Index, 
        pPlane);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetClipStatus(
      D3DCLIPSTATUS9 *pClipStatus) 
    {
      return m_pDevice->GetClipStatus(
        pClipStatus);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetCreationParameters(
      D3DDEVICE_CREATION_PARAMETERS *pParameters) 
    {
      return m_pDevice->GetCreationParameters(
        pParameters);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetCurrentTexturePalette(
      UINT *pPaletteNumber)
    {
      return m_pDevice->GetCurrentTexturePalette(
        pPaletteNumber);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetDepthStencilSurface(
      IDirect3DSurface9 **ppZStencilSurface) 
    {
      return m_pDevice->GetDepthStencilSurface(
        ppZStencilSurface);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetDeviceCaps(
      D3DCAPS9 *pCaps) 
    {
      return m_pDevice->GetDeviceCaps(
        pCaps);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetDirect3D(
      IDirect3D9 **ppD3D9) 
    {
      HRESULT Result = m_pDevice->GetDirect3D(
        ppD3D9);
      
      if (SUCCEEDED(Result))
      {
        *ppD3D9 = m_pD3D;
      }
        
      return Result;
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetDisplayMode(
      UINT iSwapChain, 
      D3DDISPLAYMODE* pMode) 
    {
      return m_pDevice->GetDisplayMode(
        iSwapChain, 
        pMode);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetFrontBufferData(
      UINT iSwapChain, 
      IDirect3DSurface9* pDestSurface) 
    {
      return m_pDevice->GetFrontBufferData(
        iSwapChain, 
        pDestSurface);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetFVF(
      DWORD* pFVF) 
    {
      return m_pDevice->GetFVF(
        pFVF);
    }
    
    void APIENTRY IDirect3DDevice9Hook::GetGammaRamp(
      UINT iSwapChain, 
      D3DGAMMARAMP* pRamp) 
    {
      m_pDevice->GetGammaRamp(
        iSwapChain, 
        pRamp);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetIndices(
      IDirect3DIndexBuffer9** ppIndexData) 
    {
      return m_pDevice->GetIndices(
        ppIndexData);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetLight(
      DWORD Index, 
      D3DLIGHT9 *pLight) 
    {
      return m_pDevice->GetLight(
        Index, 
        pLight);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetLightEnable(
      DWORD Index, 
      BOOL *pEnable) 
    {
      return m_pDevice->GetLightEnable(
        Index, 
        pEnable);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetMaterial(
      D3DMATERIAL9 *pMaterial) 
    {
      return m_pDevice->GetMaterial(
        pMaterial);
    }
    
    float APIENTRY IDirect3DDevice9Hook::GetNPatchMode() 
    {
      return m_pDevice->GetNPatchMode();
    }
    
    unsigned int APIENTRY IDirect3DDevice9Hook::GetNumberOfSwapChains() 
    {
      return m_pDevice->GetNumberOfSwapChains();
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetPaletteEntries(
      UINT PaletteNumber, 
      PALETTEENTRY *pEntries)
    {
      return m_pDevice->GetPaletteEntries(
        PaletteNumber, 
        pEntries);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetPixelShader(
      IDirect3DPixelShader9** ppShader) 
    {
      return m_pDevice->GetPixelShader(
        ppShader);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetPixelShaderConstantB(
      UINT StartRegister, 
      BOOL* pConstantData, 
      UINT BoolCount) 
    {
      return m_pDevice->GetPixelShaderConstantB(
        StartRegister, 
        pConstantData, 
        BoolCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetPixelShaderConstantF(
      UINT StartRegister, 
      float* pConstantData, 
      UINT Vector4fCount) 
    {
      return m_pDevice->GetPixelShaderConstantF(
        StartRegister, 
        pConstantData, 
        Vector4fCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetPixelShaderConstantI(
      UINT StartRegister, 
      int* pConstantData, 
      UINT Vector4iCount)
    {
      return m_pDevice->GetPixelShaderConstantI(
        StartRegister, 
        pConstantData, 
        Vector4iCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetRasterStatus(
      UINT iSwapChain, 
      D3DRASTER_STATUS* pRasterStatus) 
    {
      return m_pDevice->GetRasterStatus(
        iSwapChain, 
        pRasterStatus);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetRenderState(
      D3DRENDERSTATETYPE State, 
      DWORD *pValue) 
    {
      return m_pDevice->GetRenderState(
        State, 
        pValue);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetRenderTarget(
      DWORD RenderTargetIndex, 
      IDirect3DSurface9** ppRenderTarget) 
    {
      return m_pDevice->GetRenderTarget(
        RenderTargetIndex, 
        ppRenderTarget);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetRenderTargetData(
      IDirect3DSurface9* pRenderTarget, 
      IDirect3DSurface9* pDestSurface) 
    {
      return m_pDevice->GetRenderTargetData(
        pRenderTarget, 
        pDestSurface);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetSamplerState(
      DWORD Sampler, 
      D3DSAMPLERSTATETYPE Type, 
      DWORD* pValue) 
    {
      return m_pDevice->GetSamplerState(
        Sampler, 
        Type, 
        pValue);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetScissorRect(
      RECT* pRect) 
    {
      return m_pDevice->GetScissorRect(
        pRect);
    }
    
    BOOL APIENTRY IDirect3DDevice9Hook::GetSoftwareVertexProcessing() 
    {
      return m_pDevice->GetSoftwareVertexProcessing();
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetStreamSource(
      UINT StreamNumber, 
      IDirect3DVertexBuffer9** ppStreamData, 
      UINT* OffsetInBytes, 
      UINT* pStride) 
    {
      return m_pDevice->GetStreamSource(
        StreamNumber, 
        ppStreamData, 
        OffsetInBytes, 
        pStride);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetStreamSourceFreq(
      UINT StreamNumber, 
      UINT* Divider) 
    {
      return m_pDevice->GetStreamSourceFreq(
        StreamNumber, 
        Divider);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetSwapChain(
      UINT iSwapChain, 
      IDirect3DSwapChain9** pSwapChain)
    {
      return m_pDevice->GetSwapChain(
        iSwapChain, 
        pSwapChain);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetTexture(
      DWORD Stage, 
      IDirect3DBaseTexture9 **ppTexture) 
    {
      return m_pDevice->GetTexture(
        Stage, 
        ppTexture);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetTextureStageState(
      DWORD Stage, 
      D3DTEXTURESTAGESTATETYPE Type, 
      DWORD *pValue) 
    {
      return m_pDevice->GetTextureStageState( 
        Stage, 
        Type, 
        pValue);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetTransform(
      D3DTRANSFORMSTATETYPE State, 
      D3DMATRIX *pMatrix) 
    {
      return m_pDevice->GetTransform(
        State, 
        pMatrix);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetVertexDeclaration(
      IDirect3DVertexDeclaration9** ppDecl) 
    {
      return m_pDevice->GetVertexDeclaration(
        ppDecl);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetVertexShader(
      IDirect3DVertexShader9** ppShader) 
    {
      return m_pDevice->GetVertexShader(
        ppShader);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetVertexShaderConstantB(
      UINT StartRegister, 
      BOOL* pConstantData, 
      UINT BoolCount)
    {
      return m_pDevice->GetVertexShaderConstantB(
        StartRegister, 
        pConstantData, 
        BoolCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetVertexShaderConstantF(
      UINT StartRegister, 
      float* pConstantData, 
      UINT Vector4fCount) 
    {
      return m_pDevice->GetVertexShaderConstantF(
        StartRegister, 
        pConstantData, 
        Vector4fCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetVertexShaderConstantI(
      UINT StartRegister, 
      int* pConstantData, 
      UINT Vector4iCount)
    {
      return m_pDevice->GetVertexShaderConstantI(
        StartRegister, 
        pConstantData, 
        Vector4iCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::GetViewport(
      D3DVIEWPORT9 *pViewport) 
    {
      return m_pDevice->GetViewport(
        pViewport);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::LightEnable(
      DWORD LightIndex, 
      BOOL bEnable) 
    {
      return m_pDevice->LightEnable(
        LightIndex, 
        bEnable);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::MultiplyTransform(
      D3DTRANSFORMSTATETYPE State, 
      CONST D3DMATRIX *pMatrix) 
    {
      return m_pDevice->MultiplyTransform(
        State, 
        pMatrix);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::Present(
      CONST RECT *pSourceRect, 
      CONST RECT *pDestRect, 
      HWND hDestWindowOverride, 
      CONST RGNDATA *pDirtyRegion) 
    { 
      return m_pDevice->Present(
        pSourceRect, 
        pDestRect, 
        hDestWindowOverride, 
        pDirtyRegion);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::ProcessVertices(
      UINT SrcStartIndex, 
      UINT DestIndex, 
      UINT VertexCount, 
      IDirect3DVertexBuffer9* pDestBuffer, 
      IDirect3DVertexDeclaration9* pVertexDecl, 
      DWORD Flags) 
    {
      return m_pDevice->ProcessVertices(
        SrcStartIndex, 
        DestIndex, 
        VertexCount, 
        pDestBuffer, 
        pVertexDecl, 
        Flags);
    }
    
    ULONG APIENTRY IDirect3DDevice9Hook::Release() 
    {
      //if( --m_RefCount == 0 )
      //  m_pManager->Release();
    
      return m_pDevice->Release();
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::Reset(
      D3DPRESENT_PARAMETERS *pPresentationParameters) 
    {
      m_pRenderer->PreReset();
    
      HRESULT Result = m_pDevice->Reset(
        pPresentationParameters);
    
      if (SUCCEEDED(Result))
      {
        m_PresentParams = *pPresentationParameters;
        m_pRenderer->PostReset();
      }
    
      return Result;
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetClipPlane(
      DWORD Index, 
      CONST float *pPlane) 
    {
      return m_pDevice->SetClipPlane(
        Index, 
        pPlane);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetClipStatus(
      CONST D3DCLIPSTATUS9 *pClipStatus) 
    {
      return m_pDevice->SetClipStatus(
        pClipStatus);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetCurrentTexturePalette(
      UINT PaletteNumber) 
    {
      return m_pDevice->SetCurrentTexturePalette(
        PaletteNumber);
    }
    
    void APIENTRY IDirect3DDevice9Hook::SetCursorPosition(
      int X, 
      int Y, 
      DWORD Flags) 
    {
      m_pDevice->SetCursorPosition(
        X, 
        Y, 
        Flags);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetCursorProperties(
      UINT XHotSpot, 
      UINT YHotSpot, 
      IDirect3DSurface9 *pCursorBitmap) 
    {
      return m_pDevice->SetCursorProperties(
        XHotSpot, 
        YHotSpot, 
        pCursorBitmap);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetDepthStencilSurface(
      IDirect3DSurface9* pNewZStencil) 
    {
      return m_pDevice->SetDepthStencilSurface(
        pNewZStencil);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetDialogBoxMode(
      BOOL bEnableDialogs) 
    {
      return m_pDevice->SetDialogBoxMode(
        bEnableDialogs);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetFVF(
      DWORD FVF) 
    {
      return m_pDevice->SetFVF(
        FVF);
    }
    
    void APIENTRY IDirect3DDevice9Hook::SetGammaRamp(
      UINT iSwapChain, 
      DWORD Flags, 
      CONST D3DGAMMARAMP* pRamp)
    {
      m_pDevice->SetGammaRamp(
        iSwapChain, 
        Flags, 
        pRamp);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetIndices(
      IDirect3DIndexBuffer9* pIndexData) 
    {
      return m_pDevice->SetIndices(
        pIndexData);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetLight(
      DWORD Index, 
      CONST D3DLIGHT9 *pLight) 
    {
      return m_pDevice->SetLight(
        Index, 
        pLight);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetMaterial(
      CONST D3DMATERIAL9 *pMaterial) 
    { 
      return m_pDevice->SetMaterial(
        pMaterial);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetNPatchMode(
      float nSegments) 
    { 
      return m_pDevice->SetNPatchMode(
        nSegments);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetPaletteEntries(
      UINT PaletteNumber, 
      CONST PALETTEENTRY *pEntries) 
    {
      return m_pDevice->SetPaletteEntries(
        PaletteNumber, 
        pEntries);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetPixelShader(
      IDirect3DPixelShader9* pShader) 
    {
      return m_pDevice->SetPixelShader(
        pShader);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetPixelShaderConstantB(
      UINT StartRegister, 
      CONST BOOL* pConstantData, 
      UINT BoolCount) 
    {
      return m_pDevice->SetPixelShaderConstantB(
        StartRegister, 
        pConstantData, 
        BoolCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetPixelShaderConstantF(
      UINT StartRegister, 
      CONST float* pConstantData, 
      UINT Vector4fCount) 
    {
      return m_pDevice->SetPixelShaderConstantF(
        StartRegister, 
        pConstantData, 
        Vector4fCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetPixelShaderConstantI(
      UINT StartRegister, 
      CONST int* pConstantData, 
      UINT Vector4iCount) 
    {
      return m_pDevice->SetPixelShaderConstantI(
        StartRegister, 
        pConstantData, 
        Vector4iCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetRenderState(
      D3DRENDERSTATETYPE State, 
      DWORD Value) 
    {
      return m_pDevice->SetRenderState(
        State, 
        Value);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetRenderTarget(
      DWORD RenderTargetIndex, 
      IDirect3DSurface9* pRenderTarget) 
    {
      return m_pDevice->SetRenderTarget(
        RenderTargetIndex, 
        pRenderTarget);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetSamplerState(
      DWORD Sampler, 
      D3DSAMPLERSTATETYPE Type, 
      DWORD Value) 
    {
      return m_pDevice->SetSamplerState(
        Sampler, 
        Type, 
        Value);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetScissorRect(
      CONST RECT* pRect) 
    {
      return m_pDevice->SetScissorRect(
        pRect);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetSoftwareVertexProcessing(
      BOOL bSoftware) 
    {
      return m_pDevice->SetSoftwareVertexProcessing(
        bSoftware);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetStreamSource(
      UINT StreamNumber, 
      IDirect3DVertexBuffer9* pStreamData, 
      UINT OffsetInBytes, 
      UINT Stride) 
    {
      return m_pDevice->SetStreamSource(
        StreamNumber, 
        pStreamData, 
        OffsetInBytes, 
        Stride);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetStreamSourceFreq(
      UINT StreamNumber, 
      UINT Divider)
    { 
      return m_pDevice->SetStreamSourceFreq(
        StreamNumber, 
        Divider);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetTexture(DWORD Stage, 
      IDirect3DBaseTexture9 *pTexture) 
    {
      //IDirect3DDevice9 *dev = NULL;
      //if (pTexture != NULL && ((hkIDirect3DTexture9*)(pTexture))->
      //  GetDevice(&dev) == D3D_OK)
      //{
      //  if (dev == this)
      //  {
      //    return m_pDevice->SetTexture(Stage, ((hkIDirect3DTexture9*)
      //      (pTexture))->m_D3Dtex);
      //  }
      //}
      
      return m_pDevice->SetTexture(
        Stage, 
        pTexture);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetTextureStageState(
      DWORD Stage, 
      D3DTEXTURESTAGESTATETYPE Type, 
      DWORD Value) 
    {
      return m_pDevice->SetTextureStageState(
        Stage, 
        Type, 
        Value);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetTransform(
      D3DTRANSFORMSTATETYPE State, 
      CONST D3DMATRIX *pMatrix) 
    {
      return m_pDevice->SetTransform(
        State, 
        pMatrix);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetVertexDeclaration(
      IDirect3DVertexDeclaration9* pDecl) 
    {
      return m_pDevice->SetVertexDeclaration(
        pDecl);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetVertexShader(
      IDirect3DVertexShader9* pShader) 
    {
      return m_pDevice->SetVertexShader(
        pShader);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetVertexShaderConstantB(
      UINT StartRegister, 
      CONST BOOL* pConstantData, 
      UINT BoolCount) 
    {
      return m_pDevice->SetVertexShaderConstantB(
        StartRegister, 
        pConstantData, 
        BoolCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetVertexShaderConstantF(
      UINT StartRegister, 
      CONST float* pConstantData, 
      UINT Vector4fCount) 
    {
      return m_pDevice->SetVertexShaderConstantF(
        StartRegister, 
        pConstantData, 
        Vector4fCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetVertexShaderConstantI(
      UINT StartRegister, 
      CONST int* pConstantData, 
      UINT Vector4iCount) 
    {
      return m_pDevice->SetVertexShaderConstantI(
        StartRegister, 
        pConstantData, 
        Vector4iCount);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::SetViewport(
      CONST D3DVIEWPORT9 *pViewport) 
    {
      return m_pDevice->SetViewport(
        pViewport);
    }
    
    BOOL APIENTRY IDirect3DDevice9Hook::ShowCursor(
      BOOL bShow) 
    {
      return m_pDevice->ShowCursor(
        bShow);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::StretchRect(
      IDirect3DSurface9* pSourceSurface, 
      CONST RECT* pSourceRect, 
      IDirect3DSurface9* pDestSurface, 
      CONST RECT* pDestRect, 
      D3DTEXTUREFILTERTYPE Filter) 
    {
      return m_pDevice->StretchRect(
        pSourceSurface, 
        pSourceRect, 
        pDestSurface, 
        pDestRect, 
        Filter);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::TestCooperativeLevel() 
    {
      return m_pDevice->TestCooperativeLevel();
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::UpdateSurface(
      IDirect3DSurface9* pSourceSurface, 
      CONST RECT* pSourceRect, 
      IDirect3DSurface9* pDestinationSurface, 
      CONST POINT* pDestPoint) 
    {
      return m_pDevice->UpdateSurface(
        pSourceSurface, 
        pSourceRect, 
        pDestinationSurface, 
        pDestPoint);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::UpdateTexture(
      IDirect3DBaseTexture9 *pSourceTexture, 
      IDirect3DBaseTexture9 *pDestinationTexture) 
    {
      return m_pDevice->UpdateTexture(
        pSourceTexture, 
        pDestinationTexture);
    }
    
    HRESULT APIENTRY IDirect3DDevice9Hook::ValidateDevice(
      DWORD *pNumPasses) 
    {
      return m_pDevice->ValidateDevice(
        pNumPasses);
    }
  }
}
