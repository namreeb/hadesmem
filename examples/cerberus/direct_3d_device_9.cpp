// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "direct_3d_device_9.hpp"

#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>

#include "d3d9.hpp"

namespace hadesmem
{
namespace cerberus
{
HRESULT WINAPI Direct3DDevice9Proxy::QueryInterface(REFIID riid, void** obj)
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

ULONG WINAPI Direct3DDevice9Proxy::AddRef()
{
  refs_++;
  auto const ret = device_->AddRef();
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);
  return ret;
}

ULONG WINAPI Direct3DDevice9Proxy::Release()
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

HRESULT WINAPI Direct3DDevice9Proxy::TestCooperativeLevel()
{
  return device_->TestCooperativeLevel();
}

UINT WINAPI Direct3DDevice9Proxy::GetAvailableTextureMem()
{
  return device_->GetAvailableTextureMem();
}

HRESULT WINAPI Direct3DDevice9Proxy::EvictManagedResources()
{
  return device_->EvictManagedResources();
}

HRESULT WINAPI Direct3DDevice9Proxy::GetDirect3D(IDirect3D9** d3d9)
{
  return device_->GetDirect3D(d3d9);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetDeviceCaps(D3DCAPS9* caps)
{
  return device_->GetDeviceCaps(caps);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetDisplayMode(UINT swap_chain, D3DDISPLAYMODE* mode)
{
  return device_->GetDisplayMode(swap_chain, mode);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetCreationParameters(
  D3DDEVICE_CREATION_PARAMETERS* parameters)
{
  return device_->GetCreationParameters(parameters);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetCursorProperties(UINT x_hot_spot,
                                            UINT y_hot_spot,
                                            IDirect3DSurface9* cursor_bitmap)
{
  return device_->SetCursorProperties(x_hot_spot, y_hot_spot, cursor_bitmap);
}

void WINAPI Direct3DDevice9Proxy::SetCursorPosition(int x, int y, DWORD flags)
{
  return device_->SetCursorPosition(x, y, flags);
}

BOOL WINAPI Direct3DDevice9Proxy::ShowCursor(BOOL show)
{
  return device_->ShowCursor(show);
}

HRESULT WINAPI Direct3DDevice9Proxy::CreateAdditionalSwapChain(
  D3DPRESENT_PARAMETERS* presentation_parameters,
  IDirect3DSwapChain9** swap_chain)
{
  return device_->CreateAdditionalSwapChain(presentation_parameters,
                                            swap_chain);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetSwapChain(UINT swap_chain_index,
                                     IDirect3DSwapChain9** swap_chain)
{
  return device_->GetSwapChain(swap_chain_index, swap_chain);
}

UINT WINAPI Direct3DDevice9Proxy::GetNumberOfSwapChains()
{
  return device_->GetNumberOfSwapChains();
}

HRESULT WINAPI
  Direct3DDevice9Proxy::Reset(D3DPRESENT_PARAMETERS* presentation_params)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  auto& callbacks = GetOnResetD3D9Callbacks();
  callbacks.Run(device_, presentation_params);

  last_error_preserver.Revert();
  auto const ret = device_->Reset(presentation_params);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

HRESULT WINAPI Direct3DDevice9Proxy::Present(CONST RECT* source_rect,
                                             CONST RECT* dest_rect,
                                             HWND dest_window_override,
                                             CONST RGNDATA* dirty_region)
{
  return device_->Present(
    source_rect, dest_rect, dest_window_override, dirty_region);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetBackBuffer(UINT swap_chain,
                                      UINT back_buffer_index,
                                      D3DBACKBUFFER_TYPE type,
                                      IDirect3DSurface9** back_buffer)
{
  return device_->GetBackBuffer(
    swap_chain, back_buffer_index, type, back_buffer);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetRasterStatus(UINT swap_chain,
                                        D3DRASTER_STATUS* raster_status)
{
  return device_->GetRasterStatus(swap_chain, raster_status);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetDialogBoxMode(BOOL enable_dialogs)
{
  return device_->SetDialogBoxMode(enable_dialogs);
}

void WINAPI Direct3DDevice9Proxy::SetGammaRamp(UINT swap_chain,
                                               DWORD flags,
                                               CONST D3DGAMMARAMP* ramp)
{
  return device_->SetGammaRamp(swap_chain, flags, ramp);
}

void WINAPI
  Direct3DDevice9Proxy::GetGammaRamp(UINT swap_chain, D3DGAMMARAMP* ramp)
{
  return device_->GetGammaRamp(swap_chain, ramp);
}

HRESULT WINAPI Direct3DDevice9Proxy::CreateTexture(UINT width,
                                                   UINT height,
                                                   UINT levels,
                                                   DWORD usage,
                                                   D3DFORMAT format,
                                                   D3DPOOL pool,
                                                   IDirect3DTexture9** texture,
                                                   HANDLE* shared_handle)
{
  return device_->CreateTexture(
    width, height, levels, usage, format, pool, texture, shared_handle);
}

HRESULT WINAPI Direct3DDevice9Proxy::CreateVolumeTexture(
  UINT width,
  UINT height,
  UINT depth,
  UINT levels,
  DWORD usage,
  D3DFORMAT format,
  D3DPOOL pool,
  IDirect3DVolumeTexture9** volume_texture,
  HANDLE* shared_handle)
{
  return device_->CreateVolumeTexture(width,
                                      height,
                                      depth,
                                      levels,
                                      usage,
                                      format,
                                      pool,
                                      volume_texture,
                                      shared_handle);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::CreateCubeTexture(UINT edge_length,
                                          UINT levels,
                                          DWORD usage,
                                          D3DFORMAT format,
                                          D3DPOOL pool,
                                          IDirect3DCubeTexture9** cube_texture,
                                          HANDLE* shared_handle)
{
  return device_->CreateCubeTexture(
    edge_length, levels, usage, format, pool, cube_texture, shared_handle);
}

HRESULT WINAPI Direct3DDevice9Proxy::CreateVertexBuffer(
  UINT length,
  DWORD usage,
  DWORD fvf,
  D3DPOOL pool,
  IDirect3DVertexBuffer9** vertex_buffer,
  HANDLE* shared_handle)
{
  return device_->CreateVertexBuffer(
    length, usage, fvf, pool, vertex_buffer, shared_handle);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::CreateIndexBuffer(UINT length,
                                          DWORD usage,
                                          D3DFORMAT format,
                                          D3DPOOL pool,
                                          IDirect3DIndexBuffer9** index_buffer,
                                          HANDLE* shared_handle)
{
  return device_->CreateIndexBuffer(
    length, usage, format, pool, index_buffer, shared_handle);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::CreateRenderTarget(UINT width,
                                           UINT height,
                                           D3DFORMAT format,
                                           D3DMULTISAMPLE_TYPE multi_sample,
                                           DWORD multi_sample_quality,
                                           BOOL lockable,
                                           IDirect3DSurface9** surface,
                                           HANDLE* shared_handle)
{
  return device_->CreateRenderTarget(width,
                                     height,
                                     format,
                                     multi_sample,
                                     multi_sample_quality,
                                     lockable,
                                     surface,
                                     shared_handle);
}

HRESULT WINAPI Direct3DDevice9Proxy::CreateDepthStencilSurface(
  UINT width,
  UINT height,
  D3DFORMAT format,
  D3DMULTISAMPLE_TYPE multi_sample,
  DWORD multi_sample_quality,
  BOOL discard,
  IDirect3DSurface9** surface,
  HANDLE* shared_handle)
{
  return device_->CreateDepthStencilSurface(width,
                                            height,
                                            format,
                                            multi_sample,
                                            multi_sample_quality,
                                            discard,
                                            surface,
                                            shared_handle);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::UpdateSurface(IDirect3DSurface9* source_surface,
                                      CONST RECT* source_rect,
                                      IDirect3DSurface9* destination_surface,
                                      CONST POINT* dest_point)
{
  return device_->UpdateSurface(
    source_surface, source_rect, destination_surface, dest_point);
}

HRESULT WINAPI Direct3DDevice9Proxy::UpdateTexture(
  IDirect3DBaseTexture9* source_texture,
  IDirect3DBaseTexture9* destination_texture)
{
  return device_->UpdateTexture(source_texture, destination_texture);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetRenderTargetData(IDirect3DSurface9* render_target,
                                            IDirect3DSurface9* dest_surface)
{
  return device_->GetRenderTargetData(render_target, dest_surface);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetFrontBufferData(UINT swap_chain,
                                           IDirect3DSurface9* dest_surface)
{
  return device_->GetFrontBufferData(swap_chain, dest_surface);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::StretchRect(IDirect3DSurface9* source_surface,
                                    CONST RECT* source_rect,
                                    IDirect3DSurface9* dest_surface,
                                    CONST RECT* dest_rect,
                                    D3DTEXTUREFILTERTYPE filter)
{
  return device_->StretchRect(
    source_surface, source_rect, dest_surface, dest_rect, filter);
}

HRESULT WINAPI Direct3DDevice9Proxy::ColorFill(IDirect3DSurface9* surface,
                                               CONST RECT* rect,
                                               D3DCOLOR color)
{
  return device_->ColorFill(surface, rect, color);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::CreateOffscreenPlainSurface(UINT width,
                                                    UINT height,
                                                    D3DFORMAT format,
                                                    D3DPOOL pool,
                                                    IDirect3DSurface9** surface,
                                                    HANDLE* shared_handle)
{
  return device_->CreateOffscreenPlainSurface(
    width, height, format, pool, surface, shared_handle);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetRenderTarget(DWORD render_target_index,
                                        IDirect3DSurface9* render_target)
{
  return device_->SetRenderTarget(render_target_index, render_target);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetRenderTarget(DWORD render_target_index,
                                        IDirect3DSurface9** render_target)
{
  return device_->GetRenderTarget(render_target_index, render_target);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetDepthStencilSurface(IDirect3DSurface9* new_z_stencil)
{
  return device_->SetDepthStencilSurface(new_z_stencil);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetDepthStencilSurface(
  IDirect3DSurface9** z_stencil_surface)
{
  return device_->GetDepthStencilSurface(z_stencil_surface);
}

HRESULT WINAPI Direct3DDevice9Proxy::BeginScene()
{
  return device_->BeginScene();
}

HRESULT WINAPI Direct3DDevice9Proxy::EndScene()
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  auto& callbacks = GetOnFrameD3D9Callbacks();
  callbacks.Run(device_);

  last_error_preserver.Revert();
  auto const ret = device_->EndScene();
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

HRESULT WINAPI Direct3DDevice9Proxy::Clear(DWORD count,
                                           CONST D3DRECT* rects,
                                           DWORD flags,
                                           D3DCOLOR colour,
                                           float z,
                                           DWORD stencil)
{
  return device_->Clear(count, rects, flags, colour, z, stencil);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetTransform(D3DTRANSFORMSTATETYPE state,
                                                  CONST D3DMATRIX* matrix)
{
  return device_->SetTransform(state, matrix);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetTransform(D3DTRANSFORMSTATETYPE state,
                                                  D3DMATRIX* matrix)
{
  return device_->GetTransform(state, matrix);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::MultiplyTransform(D3DTRANSFORMSTATETYPE state,
                                          CONST D3DMATRIX* matrix)
{
  return device_->MultiplyTransform(state, matrix);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetViewport(CONST D3DVIEWPORT9* viewport)
{
  return device_->SetViewport(viewport);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetViewport(D3DVIEWPORT9* viewport)
{
  return device_->GetViewport(viewport);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetMaterial(CONST D3DMATERIAL9* material)
{
  return device_->SetMaterial(material);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetMaterial(D3DMATERIAL9* material)
{
  return device_->GetMaterial(material);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetLight(DWORD index, CONST D3DLIGHT9* light)
{
  return device_->SetLight(index, light);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetLight(DWORD index, D3DLIGHT9* light)
{
  return device_->GetLight(index, light);
}

HRESULT WINAPI Direct3DDevice9Proxy::LightEnable(DWORD index, BOOL enable)
{
  return device_->LightEnable(index, enable);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetLightEnable(DWORD index, BOOL* enable)
{
  return device_->GetLightEnable(index, enable);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetClipPlane(DWORD index, CONST float* plane)
{
  return device_->SetClipPlane(index, plane);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetClipPlane(DWORD index, float* plane)
{
  return device_->GetClipPlane(index, plane);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetRenderState(D3DRENDERSTATETYPE state, DWORD value)
{
  return device_->SetRenderState(state, value);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetRenderState(D3DRENDERSTATETYPE state, DWORD* value)
{
  return device_->GetRenderState(state, value);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::CreateStateBlock(D3DSTATEBLOCKTYPE type,
                                         IDirect3DStateBlock9** state_block)
{
  return device_->CreateStateBlock(type, state_block);
}

HRESULT WINAPI Direct3DDevice9Proxy::BeginStateBlock()
{
  return device_->BeginStateBlock();
}

HRESULT WINAPI
  Direct3DDevice9Proxy::EndStateBlock(IDirect3DStateBlock9** state_block)
{
  return device_->EndStateBlock(state_block);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetClipStatus(CONST D3DCLIPSTATUS9* clip_status)
{
  return device_->SetClipStatus(clip_status);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetClipStatus(D3DCLIPSTATUS9* clip_status)
{
  return device_->GetClipStatus(clip_status);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetTexture(DWORD stage, IDirect3DBaseTexture9** texture)
{
  return device_->GetTexture(stage, texture);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetTexture(DWORD stage, IDirect3DBaseTexture9* texture)
{
  return device_->SetTexture(stage, texture);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetTextureStageState(DWORD stage,
                                             D3DTEXTURESTAGESTATETYPE type,
                                             DWORD* value)
{
  return device_->GetTextureStageState(stage, type, value);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetTextureStageState(DWORD stage,
                                             D3DTEXTURESTAGESTATETYPE type,
                                             DWORD value)
{
  return device_->SetTextureStageState(stage, type, value);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetSamplerState(DWORD sampler,
                                                     D3DSAMPLERSTATETYPE type,
                                                     DWORD* value)
{
  return device_->GetSamplerState(sampler, type, value);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetSamplerState(DWORD sampler,
                                                     D3DSAMPLERSTATETYPE type,
                                                     DWORD value)
{
  return device_->SetSamplerState(sampler, type, value);
}

HRESULT WINAPI Direct3DDevice9Proxy::ValidateDevice(DWORD* num_passes)
{
  return device_->ValidateDevice(num_passes);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetPaletteEntries(UINT pallette_number,
                                          CONST PALETTEENTRY* entries)
{
  return device_->SetPaletteEntries(pallette_number, entries);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetPaletteEntries(UINT pallette_number,
                                                       PALETTEENTRY* entries)
{
  return device_->GetPaletteEntries(pallette_number, entries);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetCurrentTexturePalette(UINT pallette_number)
{
  return device_->SetCurrentTexturePalette(pallette_number);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetCurrentTexturePalette(UINT* pallette_number)
{
  return device_->GetCurrentTexturePalette(pallette_number);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetScissorRect(CONST RECT* rect)
{
  return device_->SetScissorRect(rect);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetScissorRect(RECT* rect)
{
  return device_->GetScissorRect(rect);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetSoftwareVertexProcessing(BOOL software)
{
  return device_->SetSoftwareVertexProcessing(software);
}

BOOL WINAPI Direct3DDevice9Proxy::GetSoftwareVertexProcessing()
{
  return device_->GetSoftwareVertexProcessing();
}

HRESULT WINAPI Direct3DDevice9Proxy::SetNPatchMode(float segments)
{
  return device_->SetNPatchMode(segments);
}

float WINAPI Direct3DDevice9Proxy::GetNPatchMode()
{
  return device_->GetNPatchMode();
}

HRESULT WINAPI
  Direct3DDevice9Proxy::DrawPrimitive(D3DPRIMITIVETYPE primitive_type,
                                      UINT start_vertex,
                                      UINT primitive_count)
{
  return device_->DrawPrimitive(primitive_type, start_vertex, primitive_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::DrawIndexedPrimitive(D3DPRIMITIVETYPE primitive_type,
                                             INT base_vertex_index,
                                             UINT min_vertex_index,
                                             UINT num_vertices,
                                             UINT start_index,
                                             UINT prim_count)
{
  return device_->DrawIndexedPrimitive(primitive_type,
                                       base_vertex_index,
                                       min_vertex_index,
                                       num_vertices,
                                       start_index,
                                       prim_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::DrawPrimitiveUP(D3DPRIMITIVETYPE primitive_type,
                                        UINT primitive_count,
                                        CONST void* vertex_stream_zero_data,
                                        UINT vertex_stream_zero_stride)
{
  return device_->DrawPrimitiveUP(primitive_type,
                                  primitive_count,
                                  vertex_stream_zero_data,
                                  vertex_stream_zero_stride);
}

HRESULT WINAPI Direct3DDevice9Proxy::DrawIndexedPrimitiveUP(
  D3DPRIMITIVETYPE primitive_type,
  UINT min_vertex_index,
  UINT num_vertices,
  UINT primitive_count,
  CONST void* index_data,
  D3DFORMAT index_data_format,
  CONST void* vertex_stream_zero_data,
  UINT vertex_stream_zero_stride)
{
  return device_->DrawIndexedPrimitiveUP(primitive_type,
                                         min_vertex_index,
                                         num_vertices,
                                         primitive_count,
                                         index_data,
                                         index_data_format,
                                         vertex_stream_zero_data,
                                         vertex_stream_zero_stride);
}

HRESULT WINAPI Direct3DDevice9Proxy::ProcessVertices(
  UINT src_start_index,
  UINT dest_index,
  UINT vertex_count,
  IDirect3DVertexBuffer9* dest_buffer,
  IDirect3DVertexDeclaration9* vertex_decl,
  DWORD flags)
{
  return device_->ProcessVertices(
    src_start_index, dest_index, vertex_count, dest_buffer, vertex_decl, flags);
}

HRESULT WINAPI Direct3DDevice9Proxy::CreateVertexDeclaration(
  CONST D3DVERTEXELEMENT9* vertex_elements, IDirect3DVertexDeclaration9** decl)
{
  return device_->CreateVertexDeclaration(vertex_elements, decl);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetVertexDeclaration(IDirect3DVertexDeclaration9* decl)
{
  return device_->SetVertexDeclaration(decl);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetVertexDeclaration(IDirect3DVertexDeclaration9** decl)
{
  return device_->GetVertexDeclaration(decl);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetFVF(DWORD fvf)
{
  return device_->SetFVF(fvf);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetFVF(DWORD* fvf)
{
  return device_->GetFVF(fvf);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::CreateVertexShader(CONST DWORD* function,
                                           IDirect3DVertexShader9** shader)
{
  return device_->CreateVertexShader(function, shader);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetVertexShader(IDirect3DVertexShader9* shader)
{
  return device_->SetVertexShader(shader);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetVertexShader(IDirect3DVertexShader9** shader)
{
  return device_->GetVertexShader(shader);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetVertexShaderConstantF(UINT start_register,
                                                 CONST float* constant_data,
                                                 UINT vector_4f_count)
{
  return device_->SetVertexShaderConstantF(
    start_register, constant_data, vector_4f_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetVertexShaderConstantF(UINT start_register,
                                                 float* constant_data,
                                                 UINT vector_4f_count)
{
  return device_->GetVertexShaderConstantF(
    start_register, constant_data, vector_4f_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetVertexShaderConstantI(UINT start_register,
                                                 CONST int* constant_data,
                                                 UINT vector_4i_count)
{
  return device_->SetVertexShaderConstantI(
    start_register, constant_data, vector_4i_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetVertexShaderConstantI(UINT start_register,
                                                 int* constant_data,
                                                 UINT vector_4i_count)
{
  return device_->GetVertexShaderConstantI(
    start_register, constant_data, vector_4i_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetVertexShaderConstantB(UINT start_register,
                                                 CONST BOOL* constant_data,
                                                 UINT bool_count)
{
  return device_->SetVertexShaderConstantB(
    start_register, constant_data, bool_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetVertexShaderConstantB(UINT start_register,
                                                 BOOL* constant_data,
                                                 UINT bool_count)
{
  return device_->GetVertexShaderConstantB(
    start_register, constant_data, bool_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetStreamSource(UINT stream_number,
                                        IDirect3DVertexBuffer9* stream_data,
                                        UINT offset_in_bytes,
                                        UINT stride)
{
  return device_->SetStreamSource(
    stream_number, stream_data, offset_in_bytes, stride);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetStreamSource(UINT stream_number,
                                        IDirect3DVertexBuffer9** stream_data,
                                        UINT* offset_in_bytes,
                                        UINT* stride)
{
  return device_->GetStreamSource(
    stream_number, stream_data, offset_in_bytes, stride);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetStreamSourceFreq(UINT stream_number, UINT setting)
{
  return device_->SetStreamSourceFreq(stream_number, setting);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetStreamSourceFreq(UINT stream_number, UINT* setting)
{
  return device_->GetStreamSourceFreq(stream_number, setting);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetIndices(IDirect3DIndexBuffer9* index_data)
{
  return device_->SetIndices(index_data);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetIndices(IDirect3DIndexBuffer9** index_data)
{
  return device_->GetIndices(index_data);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::CreatePixelShader(CONST DWORD* function,
                                          IDirect3DPixelShader9** shader)
{
  return device_->CreatePixelShader(function, shader);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetPixelShader(IDirect3DPixelShader9* shader)
{
  return device_->SetPixelShader(shader);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetPixelShader(IDirect3DPixelShader9** shader)
{
  return device_->GetPixelShader(shader);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetPixelShaderConstantF(UINT start_register,
                                                CONST float* constant_data,
                                                UINT vector_4f_count)
{
  return device_->SetPixelShaderConstantF(
    start_register, constant_data, vector_4f_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetPixelShaderConstantF(UINT start_register,
                                                float* constant_data,
                                                UINT vector_4f_count)
{
  return device_->GetPixelShaderConstantF(
    start_register, constant_data, vector_4f_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetPixelShaderConstantI(UINT start_register,
                                                CONST int* constant_data,
                                                UINT vector_4i_count)
{
  return device_->SetPixelShaderConstantI(
    start_register, constant_data, vector_4i_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetPixelShaderConstantI(UINT start_register,
                                                int* constant_data,
                                                UINT vector_4i_count)
{
  return device_->GetPixelShaderConstantI(
    start_register, constant_data, vector_4i_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::SetPixelShaderConstantB(UINT start_register,
                                                CONST BOOL* constant_data,
                                                UINT bool_count)
{
  return device_->SetPixelShaderConstantB(
    start_register, constant_data, bool_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetPixelShaderConstantB(UINT start_register,
                                                BOOL* constant_data,
                                                UINT bool_count)
{
  return device_->GetPixelShaderConstantB(
    start_register, constant_data, bool_count);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::DrawRectPatch(UINT handle,
                                      CONST float* num_segs,
                                      CONST D3DRECTPATCH_INFO* rect_patch_info)
{
  return device_->DrawRectPatch(handle, num_segs, rect_patch_info);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::DrawTriPatch(UINT handle,
                                     CONST float* num_segs,
                                     CONST D3DTRIPATCH_INFO* tri_patch_info)
{
  return device_->DrawTriPatch(handle, num_segs, tri_patch_info);
}

HRESULT WINAPI Direct3DDevice9Proxy::DeletePatch(UINT handle)
{
  return device_->DeletePatch(handle);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9** query)
{
  return device_->CreateQuery(type, query);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetConvolutionMonoKernel(UINT width,
                                                              UINT height,
                                                              float* rows,
                                                              float* columns)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->SetConvolutionMonoKernel(width, height, rows, columns);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::ComposeRects(IDirect3DSurface9* src,
                                     IDirect3DSurface9* dst,
                                     IDirect3DVertexBuffer9* src_rect_descs,
                                     UINT num_rects,
                                     IDirect3DVertexBuffer9* dst_rect_dests,
                                     D3DCOMPOSERECTSOP operation,
                                     int x_offset,
                                     int y_offset)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->ComposeRects(src,
                                 dst,
                                 src_rect_descs,
                                 num_rects,
                                 dst_rect_dests,
                                 operation,
                                 x_offset,
                                 y_offset);
}

HRESULT WINAPI Direct3DDevice9Proxy::PresentEx(CONST RECT* source_rect,
                                               CONST RECT* dest_rect,
                                               HWND dest_window_override,
                                               CONST RGNDATA* dirty_region,
                                               DWORD flags)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->PresentEx(
    source_rect, dest_rect, dest_window_override, dirty_region, flags);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetGPUThreadPriority(INT* priority)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->GetGPUThreadPriority(priority);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetGPUThreadPriority(INT priority)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->SetGPUThreadPriority(priority);
}

HRESULT WINAPI Direct3DDevice9Proxy::WaitForVBlank(UINT swap_chain)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->WaitForVBlank(swap_chain);
}

HRESULT WINAPI Direct3DDevice9Proxy::CheckResourceResidency(
  IDirect3DResource9** resource_array, UINT32 num_resources)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->CheckResourceResidency(resource_array, num_resources);
}

HRESULT WINAPI Direct3DDevice9Proxy::SetMaximumFrameLatency(UINT max_latency)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->SetMaximumFrameLatency(max_latency);
}

HRESULT WINAPI Direct3DDevice9Proxy::GetMaximumFrameLatency(UINT* max_latency)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->GetMaximumFrameLatency(max_latency);
}

HRESULT WINAPI Direct3DDevice9Proxy::CheckDeviceState(HWND destination_window)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->CheckDeviceState(destination_window);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::CreateRenderTargetEx(UINT width,
                                             UINT height,
                                             D3DFORMAT format,
                                             D3DMULTISAMPLE_TYPE multi_sample,
                                             DWORD multisample_quality,
                                             BOOL lockable,
                                             IDirect3DSurface9** surface,
                                             HANDLE* shared_handle,
                                             DWORD usage)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->CreateRenderTargetEx(width,
                                         height,
                                         format,
                                         multi_sample,
                                         multisample_quality,
                                         lockable,
                                         surface,
                                         shared_handle,
                                         usage);
}

HRESULT WINAPI Direct3DDevice9Proxy::CreateOffscreenPlainSurfaceEx(
  UINT width,
  UINT height,
  D3DFORMAT format,
  D3DPOOL pool,
  IDirect3DSurface9** surface,
  HANDLE* shared_handle,
  DWORD usage)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->CreateOffscreenPlainSurfaceEx(
    width, height, format, pool, surface, shared_handle, usage);
}

HRESULT WINAPI Direct3DDevice9Proxy::CreateDepthStencilSurfaceEx(
  UINT width,
  UINT height,
  D3DFORMAT format,
  D3DMULTISAMPLE_TYPE multi_sample,
  DWORD multisample_quality,
  BOOL discard,
  IDirect3DSurface9** surface,
  HANDLE* shared_handle,
  DWORD usage)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->CreateDepthStencilSurfaceEx(width,
                                                height,
                                                format,
                                                multi_sample,
                                                multisample_quality,
                                                discard,
                                                surface,
                                                shared_handle,
                                                usage);
}

HRESULT WINAPI
  Direct3DDevice9Proxy::ResetEx(D3DPRESENT_PARAMETERS* presentation_parameters,
                                D3DDISPLAYMODEEX* fullscreen_display_mode)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  auto& callbacks = GetOnResetD3D9Callbacks();
  callbacks.Run(device_, presentation_parameters);

  last_error_preserver.Revert();
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  auto const ret =
    device_ex->ResetEx(presentation_parameters, fullscreen_display_mode);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

HRESULT WINAPI
  Direct3DDevice9Proxy::GetDisplayModeEx(UINT swap_chain,
                                         D3DDISPLAYMODEEX* mode,
                                         D3DDISPLAYROTATION* rotation)
{
  auto const device_ex = static_cast<IDirect3DDevice9Ex*>(device_);
  return device_ex->GetDisplayModeEx(swap_chain, mode, rotation);
}

void Direct3DDevice9Proxy::Cleanup()
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  auto& callbacks = GetOnReleaseD3D9Callbacks();
  callbacks.Run(device_);
}
}
}
