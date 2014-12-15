// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>

#include <windows.h>

#include <d3d9.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif // #if defined(HADESMEM_GCC)

class Direct3DDevice9Proxy : public IDirect3DDevice9Ex
{
public:
  explicit Direct3DDevice9Proxy(IDirect3DDevice9* device) : device_{device}
  {
  }

  // IUnknown
  HRESULT WINAPI QueryInterface(REFIID riid, void** obj) override;
  ULONG WINAPI AddRef() override;
  ULONG WINAPI Release() override;

  // IDirect3DDevice9
  HRESULT WINAPI TestCooperativeLevel() override;
  UINT WINAPI GetAvailableTextureMem() override;
  HRESULT WINAPI EvictManagedResources() override;
  HRESULT WINAPI GetDirect3D(IDirect3D9** d3d9) override;
  HRESULT WINAPI GetDeviceCaps(D3DCAPS9* caps) override;
  HRESULT WINAPI GetDisplayMode(UINT swap_chain, D3DDISPLAYMODE* mode) override;
  HRESULT WINAPI
    GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* parameters) override;
  HRESULT WINAPI SetCursorProperties(UINT x_hot_spot,
                                     UINT y_hot_spot,
                                     IDirect3DSurface9* cursor_bitmap) override;
  void WINAPI SetCursorPosition(int x, int y, DWORD flags) override;
  BOOL WINAPI ShowCursor(BOOL show) override;
  HRESULT WINAPI
    CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* presentation_parameters,
                              IDirect3DSwapChain9** swap_chain) override;
  HRESULT WINAPI GetSwapChain(UINT swap_chain_index,
                              IDirect3DSwapChain9** swap_chain) override;
  UINT WINAPI GetNumberOfSwapChains() override;
  HRESULT WINAPI Reset(D3DPRESENT_PARAMETERS* presentation_params) override;
  HRESULT WINAPI Present(CONST RECT* source_rect,
                         CONST RECT* dest_rect,
                         HWND dest_window_override,
                         CONST RGNDATA* dirty_region) override;
  HRESULT WINAPI GetBackBuffer(UINT swap_chain,
                               UINT back_buffer_index,
                               D3DBACKBUFFER_TYPE type,
                               IDirect3DSurface9** back_buffer) override;
  HRESULT WINAPI
    GetRasterStatus(UINT swap_chain, D3DRASTER_STATUS* raster_status) override;
  HRESULT WINAPI SetDialogBoxMode(BOOL enable_dialogs) override;
  void WINAPI SetGammaRamp(UINT swap_chain,
                           DWORD flags,
                           CONST D3DGAMMARAMP* ramp) override;
  void WINAPI GetGammaRamp(UINT swap_chain, D3DGAMMARAMP* ramp) override;
  HRESULT WINAPI CreateTexture(UINT width,
                               UINT height,
                               UINT levels,
                               DWORD usage,
                               D3DFORMAT format,
                               D3DPOOL pool,
                               IDirect3DTexture9** texture,
                               HANDLE* shared_handle) override;
  HRESULT WINAPI CreateVolumeTexture(UINT width,
                                     UINT height,
                                     UINT depth,
                                     UINT levels,
                                     DWORD usage,
                                     D3DFORMAT format,
                                     D3DPOOL pool,
                                     IDirect3DVolumeTexture9** volume_texture,
                                     HANDLE* shared_handle) override;
  HRESULT WINAPI CreateCubeTexture(UINT edge_length,
                                   UINT levels,
                                   DWORD usage,
                                   D3DFORMAT format,
                                   D3DPOOL pool,
                                   IDirect3DCubeTexture9** cube_texture,
                                   HANDLE* shared_handle) override;
  HRESULT WINAPI CreateVertexBuffer(UINT length,
                                    DWORD usage,
                                    DWORD fvf,
                                    D3DPOOL pool,
                                    IDirect3DVertexBuffer9** vertex_buffer,
                                    HANDLE* shared_handle) override;
  HRESULT WINAPI CreateIndexBuffer(UINT length,
                                   DWORD usage,
                                   D3DFORMAT format,
                                   D3DPOOL pool,
                                   IDirect3DIndexBuffer9** index_buffer,
                                   HANDLE* shared_handle) override;
  HRESULT WINAPI CreateRenderTarget(UINT width,
                                    UINT height,
                                    D3DFORMAT format,
                                    D3DMULTISAMPLE_TYPE multi_sample,
                                    DWORD multi_sample_quality,
                                    BOOL lockable,
                                    IDirect3DSurface9** surface,
                                    HANDLE* shared_handle) override;
  HRESULT WINAPI CreateDepthStencilSurface(UINT width,
                                           UINT height,
                                           D3DFORMAT format,
                                           D3DMULTISAMPLE_TYPE multi_sample,
                                           DWORD multi_sample_quality,
                                           BOOL discard,
                                           IDirect3DSurface9** surface,
                                           HANDLE* shared_handle) override;
  HRESULT WINAPI UpdateSurface(IDirect3DSurface9* source_surface,
                               CONST RECT* source_rect,
                               IDirect3DSurface9* destination_surface,
                               CONST POINT* dest_point) override;
  HRESULT WINAPI
    UpdateTexture(IDirect3DBaseTexture9* source_texture,
                  IDirect3DBaseTexture9* destination_texture) override;
  HRESULT WINAPI GetRenderTargetData(IDirect3DSurface9* render_target,
                                     IDirect3DSurface9* dest_surface) override;
  HRESULT WINAPI GetFrontBufferData(UINT swap_chain,
                                    IDirect3DSurface9* dest_surface) override;
  HRESULT WINAPI StretchRect(IDirect3DSurface9* source_surface,
                             CONST RECT* source_rect,
                             IDirect3DSurface9* dest_surface,
                             CONST RECT* dest_rect,
                             D3DTEXTUREFILTERTYPE filter) override;
  HRESULT WINAPI ColorFill(IDirect3DSurface9* surface,
                           CONST RECT* rect,
                           D3DCOLOR color) override;
  HRESULT WINAPI CreateOffscreenPlainSurface(UINT width,
                                             UINT height,
                                             D3DFORMAT format,
                                             D3DPOOL pool,
                                             IDirect3DSurface9** surface,
                                             HANDLE* shared_handle) override;
  HRESULT WINAPI SetRenderTarget(DWORD render_target_index,
                                 IDirect3DSurface9* render_target) override;
  HRESULT WINAPI GetRenderTarget(DWORD render_target_index,
                                 IDirect3DSurface9** render_target) override;
  HRESULT WINAPI
    SetDepthStencilSurface(IDirect3DSurface9* new_z_stencil) override;
  HRESULT WINAPI
    GetDepthStencilSurface(IDirect3DSurface9** z_stencil_surface) override;
  HRESULT WINAPI BeginScene() override;
  HRESULT WINAPI EndScene() override;
  HRESULT WINAPI Clear(DWORD count,
                       CONST D3DRECT* rects,
                       DWORD flags,
                       D3DCOLOR colour,
                       float z,
                       DWORD stencil) override;
  HRESULT WINAPI
    SetTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* matrix) override;
  HRESULT WINAPI
    GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX* matrix) override;
  HRESULT WINAPI MultiplyTransform(D3DTRANSFORMSTATETYPE state,
                                   CONST D3DMATRIX* matrix) override;
  HRESULT WINAPI SetViewport(CONST D3DVIEWPORT9* viewport) override;
  HRESULT WINAPI GetViewport(D3DVIEWPORT9* viewport) override;
  HRESULT WINAPI SetMaterial(CONST D3DMATERIAL9* material) override;
  HRESULT WINAPI GetMaterial(D3DMATERIAL9* material) override;
  HRESULT WINAPI SetLight(DWORD index, CONST D3DLIGHT9* light) override;
  HRESULT WINAPI GetLight(DWORD index, D3DLIGHT9* light) override;
  HRESULT WINAPI LightEnable(DWORD index, BOOL enable) override;
  HRESULT WINAPI GetLightEnable(DWORD index, BOOL* enable) override;
  HRESULT WINAPI SetClipPlane(DWORD index, CONST float* plane) override;
  HRESULT WINAPI GetClipPlane(DWORD index, float* plane) override;
  HRESULT WINAPI SetRenderState(D3DRENDERSTATETYPE state, DWORD value) override;
  HRESULT WINAPI
    GetRenderState(D3DRENDERSTATETYPE state, DWORD* value) override;
  HRESULT WINAPI CreateStateBlock(D3DSTATEBLOCKTYPE type,
                                  IDirect3DStateBlock9** state_block) override;
  HRESULT WINAPI BeginStateBlock() override;
  HRESULT WINAPI EndStateBlock(IDirect3DStateBlock9** state_block) override;
  HRESULT WINAPI SetClipStatus(CONST D3DCLIPSTATUS9* clip_status) override;
  HRESULT WINAPI GetClipStatus(D3DCLIPSTATUS9* clip_status) override;
  HRESULT WINAPI
    GetTexture(DWORD stage, IDirect3DBaseTexture9** texture) override;
  HRESULT WINAPI
    SetTexture(DWORD stage, IDirect3DBaseTexture9* texture) override;
  HRESULT WINAPI GetTextureStageState(DWORD stage,
                                      D3DTEXTURESTAGESTATETYPE type,
                                      DWORD* value) override;
  HRESULT WINAPI SetTextureStageState(DWORD stage,
                                      D3DTEXTURESTAGESTATETYPE type,
                                      DWORD value) override;
  HRESULT WINAPI GetSamplerState(DWORD sampler,
                                 D3DSAMPLERSTATETYPE type,
                                 DWORD* value) override;
  HRESULT WINAPI SetSamplerState(DWORD sampler,
                                 D3DSAMPLERSTATETYPE type,
                                 DWORD value) override;
  HRESULT WINAPI ValidateDevice(DWORD* num_passes) override;
  HRESULT WINAPI SetPaletteEntries(UINT pallette_number,
                                   CONST PALETTEENTRY* entries) override;
  HRESULT WINAPI
    GetPaletteEntries(UINT pallette_number, PALETTEENTRY* entries) override;
  HRESULT WINAPI SetCurrentTexturePalette(UINT pallette_number) override;
  HRESULT WINAPI GetCurrentTexturePalette(UINT* pallette_number) override;
  HRESULT WINAPI SetScissorRect(CONST RECT* rect) override;
  HRESULT WINAPI GetScissorRect(RECT* rect) override;
  HRESULT WINAPI SetSoftwareVertexProcessing(BOOL software) override;
  BOOL WINAPI GetSoftwareVertexProcessing() override;
  HRESULT WINAPI SetNPatchMode(float segments) override;
  float WINAPI GetNPatchMode() override;
  HRESULT WINAPI DrawPrimitive(D3DPRIMITIVETYPE primitive_type,
                               UINT start_vertex,
                               UINT primitive_count) override;
  HRESULT WINAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE primitive_type,
                                      INT base_vertex_index,
                                      UINT min_vertex_index,
                                      UINT num_vertices,
                                      UINT start_index,
                                      UINT prim_count) override;
  HRESULT WINAPI DrawPrimitiveUP(D3DPRIMITIVETYPE primitive_type,
                                 UINT primitive_count,
                                 CONST void* vertex_stream_zero_data,
                                 UINT vertex_stream_zero_stride) override;
  HRESULT WINAPI
    DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE primitive_type,
                           UINT min_vertex_index,
                           UINT num_vertices,
                           UINT primitive_count,
                           CONST void* index_data,
                           D3DFORMAT index_data_format,
                           CONST void* vertex_stream_zero_data,
                           UINT vertex_stream_zero_stride) override;
  HRESULT WINAPI ProcessVertices(UINT src_start_index,
                                 UINT dest_index,
                                 UINT vertex_count,
                                 IDirect3DVertexBuffer9* dest_buffer,
                                 IDirect3DVertexDeclaration9* vertex_decl,
                                 DWORD flags) override;
  HRESULT WINAPI
    CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* vertex_elements,
                            IDirect3DVertexDeclaration9** decl) override;
  HRESULT WINAPI
    SetVertexDeclaration(IDirect3DVertexDeclaration9* decl) override;
  HRESULT WINAPI
    GetVertexDeclaration(IDirect3DVertexDeclaration9** decl) override;
  HRESULT WINAPI SetFVF(DWORD fvf) override;
  HRESULT WINAPI GetFVF(DWORD* fvf) override;
  HRESULT WINAPI CreateVertexShader(CONST DWORD* function,
                                    IDirect3DVertexShader9** shader) override;
  HRESULT WINAPI SetVertexShader(IDirect3DVertexShader9* shader) override;
  HRESULT WINAPI GetVertexShader(IDirect3DVertexShader9** shader) override;
  HRESULT WINAPI SetVertexShaderConstantF(UINT start_register,
                                          CONST float* constant_data,
                                          UINT vector_4f_count) override;
  HRESULT WINAPI GetVertexShaderConstantF(UINT start_register,
                                          float* constant_data,
                                          UINT vector_4f_count) override;
  HRESULT WINAPI SetVertexShaderConstantI(UINT start_register,
                                          CONST int* constant_data,
                                          UINT vector_4i_count) override;
  HRESULT WINAPI GetVertexShaderConstantI(UINT start_register,
                                          int* constant_data,
                                          UINT vector_4i_count) override;
  HRESULT WINAPI SetVertexShaderConstantB(UINT start_register,
                                          CONST BOOL* constant_data,
                                          UINT bool_count) override;
  HRESULT WINAPI GetVertexShaderConstantB(UINT start_register,
                                          BOOL* constant_data,
                                          UINT bool_count) override;
  HRESULT WINAPI SetStreamSource(UINT stream_number,
                                 IDirect3DVertexBuffer9* stream_data,
                                 UINT offset_in_bytes,
                                 UINT stride) override;
  HRESULT WINAPI GetStreamSource(UINT stream_number,
                                 IDirect3DVertexBuffer9** stream_data,
                                 UINT* offset_in_bytes,
                                 UINT* stride) override;
  HRESULT WINAPI SetStreamSourceFreq(UINT stream_number, UINT setting) override;
  HRESULT WINAPI
    GetStreamSourceFreq(UINT stream_number, UINT* setting) override;
  HRESULT WINAPI SetIndices(IDirect3DIndexBuffer9* index_data) override;
  HRESULT WINAPI GetIndices(IDirect3DIndexBuffer9** index_data) override;
  HRESULT WINAPI CreatePixelShader(CONST DWORD* function,
                                   IDirect3DPixelShader9** shader) override;
  HRESULT WINAPI SetPixelShader(IDirect3DPixelShader9* shader) override;
  HRESULT WINAPI GetPixelShader(IDirect3DPixelShader9** shader) override;
  HRESULT WINAPI SetPixelShaderConstantF(UINT start_register,
                                         CONST float* constant_data,
                                         UINT vector_4f_count) override;
  HRESULT WINAPI GetPixelShaderConstantF(UINT start_register,
                                         float* constant_data,
                                         UINT vector_4f_count) override;
  HRESULT WINAPI SetPixelShaderConstantI(UINT start_register,
                                         CONST int* constant_data,
                                         UINT vector_4i_count) override;
  HRESULT WINAPI GetPixelShaderConstantI(UINT start_register,
                                         int* constant_data,
                                         UINT vector_4i_count) override;
  HRESULT WINAPI SetPixelShaderConstantB(UINT start_register,
                                         CONST BOOL* constant_data,
                                         UINT bool_count) override;
  HRESULT WINAPI GetPixelShaderConstantB(UINT start_register,
                                         BOOL* constant_data,
                                         UINT bool_count) override;
  HRESULT WINAPI
    DrawRectPatch(UINT handle,
                  CONST float* num_segs,
                  CONST D3DRECTPATCH_INFO* rect_patch_info) override;
  HRESULT WINAPI DrawTriPatch(UINT handle,
                              CONST float* num_segs,
                              CONST D3DTRIPATCH_INFO* tri_patch_info) override;
  HRESULT WINAPI DeletePatch(UINT handle) override;
  HRESULT WINAPI
    CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9** query) override;

  // IDirect3DDevice9Ex
  HRESULT WINAPI SetConvolutionMonoKernel(UINT width,
                                          UINT height,
                                          float* rows,
                                          float* columns) override;
  HRESULT WINAPI ComposeRects(IDirect3DSurface9* src,
                              IDirect3DSurface9* dst,
                              IDirect3DVertexBuffer9* src_rect_descs,
                              UINT num_rects,
                              IDirect3DVertexBuffer9* dst_rect_dests,
                              D3DCOMPOSERECTSOP operation,
                              int x_offset,
                              int y_offset) override;
  HRESULT WINAPI PresentEx(CONST RECT* source_rect,
                           CONST RECT* dest_rect,
                           HWND dest_window_override,
                           CONST RGNDATA* dirty_region,
                           DWORD flags) override;
  HRESULT WINAPI GetGPUThreadPriority(INT* priority) override;
  HRESULT WINAPI SetGPUThreadPriority(INT priority) override;
  HRESULT WINAPI WaitForVBlank(UINT swap_chain) override;
  HRESULT WINAPI CheckResourceResidency(IDirect3DResource9** resource_array,
                                        UINT32 num_resources) override;
  HRESULT WINAPI SetMaximumFrameLatency(UINT max_latency) override;
  HRESULT WINAPI GetMaximumFrameLatency(UINT* max_latency) override;
  HRESULT WINAPI CheckDeviceState(HWND destination_window) override;
  HRESULT WINAPI CreateRenderTargetEx(UINT width,
                                      UINT height,
                                      D3DFORMAT format,
                                      D3DMULTISAMPLE_TYPE multi_sample,
                                      DWORD multisample_quality,
                                      BOOL lockable,
                                      IDirect3DSurface9** surface,
                                      HANDLE* shared_handle,
                                      DWORD usage) override;
  HRESULT WINAPI CreateOffscreenPlainSurfaceEx(UINT width,
                                               UINT height,
                                               D3DFORMAT format,
                                               D3DPOOL pool,
                                               IDirect3DSurface9** surface,
                                               HANDLE* shared_handle,
                                               DWORD usage) override;
  HRESULT WINAPI CreateDepthStencilSurfaceEx(UINT width,
                                             UINT height,
                                             D3DFORMAT format,
                                             D3DMULTISAMPLE_TYPE multi_sample,
                                             DWORD multisample_quality,
                                             BOOL discard,
                                             IDirect3DSurface9** surface,
                                             HANDLE* shared_handle,
                                             DWORD usage) override;
  HRESULT WINAPI ResetEx(D3DPRESENT_PARAMETERS* presentation_parameters,
                         D3DDISPLAYMODEEX* fullscreen_display_mode) override;
  HRESULT WINAPI GetDisplayModeEx(UINT swap_chain,
                                  D3DDISPLAYMODEEX* mode,
                                  D3DDISPLAYROTATION* rotation) override;

protected:
  void Cleanup();

  std::int64_t refs_{1};
  IDirect3DDevice9* device_{};
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)
}
}
