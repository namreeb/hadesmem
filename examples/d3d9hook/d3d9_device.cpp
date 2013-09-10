// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d9_device.hpp"

#include <algorithm>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/thread.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <winternl.h>
#include <winnt.h>

#include <hadesmem/error.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/make_unique.hpp>

#include "proxy_call.hpp"

// TODO: Clean this all up.

// TODO: Add more error checking.

// TODO: Support separately created swap chain.

// TODO: Support IDirect3DDevice9Ex.

// TODO: Thread safety and (if applicable) reentrancy safety checks.

// TODO: Ensure that unloading on the fly does not leak a proxy or any other 
// resources.

// TODO: Support IDirect3DSwapChain9.

// TODO: Decouple all the non-essential logic from this component. It should 
// be implemented separately, via a callback mechanism or equivalent. 

// TODO: Add reset detection to ensure we don't call device members while 
// attempting a reset.

// TODO: Add checks to resource cleanup code to ensure that we don't have 
// any calls in progress.

// TODO: Add a reference to the underlying device to avoid it being pulled 
// out from underneath us? Is this necessary? Any other reasons to AddRef?

// TODO: Add checks on whether object is valid before making a call to the 
// underlying device. This will require NOT using the 'delete this' trick in 
// Release though... Investigate this.

IDirect3DDevice9Proxy::IDirect3DDevice9Proxy(IDirect3DDevice9* ppdevice)
  : device_(ppdevice), 
  render_data_(), 
  ref_count_(1), 
  magic_(0xDEADBEEF), 
  call_count_(0)
{
  try
  {
    HADESMEM_DETAIL_TRACE_A("Creating RenderData for device.");

    auto const proxy_call = MakeProxyCall(this);

    HRESULT const create_state_block_hr = device_->CreateStateBlock(
      D3DSBT_ALL, &render_data_.state_block);
    if (FAILED(create_state_block_hr))
    {
      HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("GetProcAddress failed.") << 
        hadesmem::ErrorCodeWinHr(create_state_block_hr));
    }

    render_data_.default_state_block = CreateDefaultStateBlock();
    if (!render_data_.default_state_block)
    {
      HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("CreateDefaultStateBlock failed."));
    }

    if (!d3dx_mod_)
    {
      d3dx_mod_ = 
        hadesmem::detail::make_unique<hadesmem::detail::SmartModuleHandle>(
        ::LoadLibraryW(L"d3dx9_43"));
    }

    if (!d3dx_mod_->GetHandle())
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("LoadLibraryW failed.") << 
        hadesmem::ErrorCodeWinLast(last_error));
    }
      
    auto const d3dx_create_line = 
      reinterpret_cast<decltype(&D3DXCreateLine)>(
      ::GetProcAddress(d3dx_mod_->GetHandle(), 
      "D3DXCreateLine"));
    if (!d3dx_create_line)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("GetProcAddress failed.\n") << 
        hadesmem::ErrorCodeWinLast(last_error));
    }
      
    HRESULT const line_hr = d3dx_create_line(device_, &render_data_.line);
    if (FAILED(line_hr))
    {
      BOOST_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("Failed to create line.") << 
        hadesmem::ErrorCodeWinHr(line_hr));
    }

    std::wstring const face_name(L"Tahoma");
    DWORD const height = 14;
      
    auto const d3dx_create_font = 
      reinterpret_cast<decltype(&D3DXCreateFontW)>(
      ::GetProcAddress(d3dx_mod_->GetHandle(), 
      "D3DXCreateFontW"));
    if (!d3dx_create_font)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("GetProcAddress failed.") << 
        hadesmem::ErrorCodeWinLast(last_error));
    }
      
    HRESULT const font_hr = d3dx_create_font(
      device_, 
      -MulDiv(height, ::GetDeviceCaps(GetDC(0), LOGPIXELSY), 72), 
      0, 
      FW_NORMAL, 
      0, 
      0, 
      DEFAULT_CHARSET, 
      OUT_DEFAULT_PRECIS, 
      DEFAULT_QUALITY, 
      DEFAULT_PITCH | FF_DONTCARE, 
      face_name.c_str(), 
      &render_data_.font);
    if (FAILED(font_hr))
    {
      BOOST_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("Failed to create font.") << 
        hadesmem::ErrorCodeWinHr(font_hr));
    }
      
    HRESULT const preload_characters_hr = render_data_.font->PreloadCharacters(
      0, 255);
    if (FAILED(preload_characters_hr))
    {
      BOOST_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("Failed to preload characters.") << 
        hadesmem::ErrorCodeWinHr(preload_characters_hr));
    }

    Validate();
  }
  catch (std::exception const& /*e*/)
  {
    HADESMEM_DETAIL_TRACE_A("Failed while initializing render data.");
    
    if (render_data_.state_block)
    {
      render_data_.state_block->Release();
      render_data_.state_block = nullptr;
    }
        
    if (render_data_.default_state_block)
    {
      render_data_.default_state_block->Release();
      render_data_.default_state_block = nullptr;
    }

    if (render_data_.line)
    {
      render_data_.line->Release();
      render_data_.line = nullptr;
    }
  
    if (render_data_.font)
    {
      render_data_.font->Release();
      render_data_.font = nullptr;
    }

    throw;
  }
}

IDirect3DDevice9Proxy::~IDirect3DDevice9Proxy()
{
  HADESMEM_DETAIL_ASSERT(magic_ == 0xDEADBEEF);
  HADESMEM_DETAIL_ASSERT(ref_count_ == 0);
  HADESMEM_DETAIL_ASSERT(call_count_ == 0);
  magic_ = 0xCAFEBABE;
}

// IUnknown methods
HRESULT __stdcall IDirect3DDevice9Proxy::QueryInterface(REFIID riid, void** ppvobj)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  // TODO: Handle this API properly, we are missing hooks here.
  return device_->QueryInterface(riid, ppvobj);
}
  
ULONG __stdcall IDirect3DDevice9Proxy::AddRef()
{
  Validate();
  HADESMEM_DETAIL_ASSERT(ref_count_ != 0);
  ++ref_count_;
  auto const proxy_call = MakeProxyCall(this);
  return device_->AddRef();
}
  
ULONG __stdcall IDirect3DDevice9Proxy::Release()
{
  Validate();

  bool cleanup_this = false;
  ULONG new_ref = --ref_count_;

  try
  {
    if (new_ref == 0)
    {
      OnRelease();
      cleanup_this = true;
    }
    
    ULONG ret = 0;
    {
      auto const proxy_call = MakeProxyCall(this);
      ret = device_->Release();
    }

    if (cleanup_this)
    {
      HADESMEM_DETAIL_TRACE_A("Deleting IDirect3DDevice9Proxy.");

      delete this;
    }

    return ret;
  }
  catch (std::exception const& e)
  {
    HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());
  }
  
  return new_ref;
}
  
// IDirect3DDevice9 methods
HRESULT __stdcall IDirect3DDevice9Proxy::TestCooperativeLevel()
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->TestCooperativeLevel();
}
  
UINT __stdcall IDirect3DDevice9Proxy::GetAvailableTextureMem()
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetAvailableTextureMem();
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::EvictManagedResources()
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->EvictManagedResources();
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetDirect3D(IDirect3D9** ppd3d9)
{
  Validate();
  // TODO: Direct3D interface interception.
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetDirect3D(ppd3d9);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetDeviceCaps(D3DCAPS9* pcaps)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetDeviceCaps(pcaps);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetDisplayMode(UINT iswapchain, D3DDISPLAYMODE* pmode)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetDisplayMode(iswapchain, pmode);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pparameters)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetCreationParameters(pparameters);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetCursorProperties(UINT x_hot_spot, UINT y_hot_spot, IDirect3DSurface9* pcursorbitmap)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetCursorProperties(x_hot_spot, y_hot_spot, pcursorbitmap);
}
  
void __stdcall IDirect3DDevice9Proxy::SetCursorPosition(int x, int y, DWORD flags)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetCursorPosition(x, y, flags);
}
  
BOOL __stdcall IDirect3DDevice9Proxy::ShowCursor(BOOL bshow)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->ShowCursor(bshow);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* ppresentationparameters, IDirect3DSwapChain9** pswapchain)
{
  Validate();
  // TODO: Handle this API properly, we are missing hooks.
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateAdditionalSwapChain(ppresentationparameters, pswapchain);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetSwapChain(UINT iswapchain, IDirect3DSwapChain9** pswapchain)
{
  Validate();
  // TODO: Handle this API properly, we are missing hooks.
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetSwapChain(iswapchain, pswapchain);
}
  
UINT __stdcall IDirect3DDevice9Proxy::GetNumberOfSwapChains()
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetNumberOfSwapChains();
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
  Validate();

  HADESMEM_DETAIL_TRACE_A("Reset called.");

  try
  {
    OnPreReset();
  }
  catch (std::exception const& e)
  {
    HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());
  }

  HRESULT hr = device_->Reset(pPresentationParameters);
  DWORD const last_error = ::GetLastError();

  HADESMEM_DETAIL_TRACE_A(SUCCEEDED(hr) ? 
    "Reset succeeded." : 
    "Reset failed.");

  try
  {
    OnPostReset();
  }
  catch (std::exception const& e)
  {
    HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());
  }

  ::SetLastError(last_error);
  return hr;
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::Present(CONST RECT* psourcerect, CONST RECT* pdestrect, HWND hdestwindowoverride, CONST RGNDATA* pdirtyregion)
{
  Validate();
  // TODO: Support for hooking OnFrame from here instead of EndScene.
  auto const proxy_call = MakeProxyCall(this);
  return device_->Present(psourcerect, pdestrect, hdestwindowoverride, pdirtyregion);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetBackBuffer(UINT iswapchain, UINT ibackbuffer, D3DBACKBUFFER_TYPE type, IDirect3DSurface9** ppbackbuffer)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetBackBuffer(iswapchain, ibackbuffer, type, ppbackbuffer);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetRasterStatus(UINT iswapchain, D3DRASTER_STATUS* prasterstatus)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetRasterStatus(iswapchain, prasterstatus);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetDialogBoxMode(BOOL benabledialogs)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetDialogBoxMode(benabledialogs);
}
  
void __stdcall IDirect3DDevice9Proxy::SetGammaRamp(UINT iswapchain, DWORD flags, CONST D3DGAMMARAMP* pramp)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetGammaRamp(iswapchain, flags, pramp);
}
  
void __stdcall IDirect3DDevice9Proxy::GetGammaRamp(UINT iswapchain, D3DGAMMARAMP* pramp)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetGammaRamp(iswapchain, pramp);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9** pptexture, HANDLE* psharedhandle)
{
  Validate();
  // TODO: Texture hooking support.
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateTexture(width, height, levels, usage, format, pool, pptexture, psharedhandle);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateVolumeTexture(UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DVolumeTexture9** ppvolumetexture, HANDLE* psharedhandle)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateVolumeTexture(width, height, depth, levels, usage, format, pool, ppvolumetexture, psharedhandle);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateCubeTexture(UINT edgelength, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture9** ppcubetexture, HANDLE* psharedhandle)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateCubeTexture(edgelength, levels, usage, format, pool, ppcubetexture, psharedhandle);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateVertexBuffer(UINT Length, DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer9** ppvertexbuffer, HANDLE* psharedhandle)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateVertexBuffer(Length, usage, fvf, pool, ppvertexbuffer, psharedhandle);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateIndexBuffer(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer9** ppindexbuffer, HANDLE* psharedhandle)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateIndexBuffer(length, usage, format, pool, ppindexbuffer, psharedhandle);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateRenderTarget(UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisamplequality, BOOL lockable, IDirect3DSurface9** ppsurface, HANDLE* psharedhandle)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateRenderTarget(width, height, format, multisample, multisamplequality, lockable, ppsurface, psharedhandle);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateDepthStencilSurface(UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisamplequality, BOOL discard, IDirect3DSurface9** ppsurface, HANDLE* psharedhandle)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateDepthStencilSurface(width, height, format, multisample, multisamplequality, discard, ppsurface, psharedhandle);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::UpdateSurface(IDirect3DSurface9* psourcesurface, CONST RECT* psourcerect, IDirect3DSurface9* pdestinationsurface, CONST POINT* pdestpoint)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->UpdateSurface(psourcesurface, psourcerect, pdestinationsurface, pdestpoint);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::UpdateTexture(IDirect3DBaseTexture9* psourcetexture, IDirect3DBaseTexture9* pdestinationtexture)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->UpdateTexture(psourcetexture, pdestinationtexture);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetRenderTargetData(IDirect3DSurface9* prendertarget, IDirect3DSurface9* pdestsurface)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetRenderTargetData(prendertarget, pdestsurface);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetFrontBufferData(UINT iswapchain, IDirect3DSurface9* pdestsurface)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetFrontBufferData(iswapchain, pdestsurface);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::StretchRect(IDirect3DSurface9* psourcesurface, CONST RECT* psourcerect, IDirect3DSurface9* pdestsurface, CONST RECT* pdestrect, D3DTEXTUREFILTERTYPE filter)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->StretchRect(psourcesurface, psourcerect, pdestsurface, pdestrect, filter);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::ColorFill(IDirect3DSurface9* psurface, CONST RECT* prect, D3DCOLOR color)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->ColorFill(psurface, prect, color);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateOffscreenPlainSurface(UINT width, UINT height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9** ppsurface, HANDLE* psharedhandle)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateOffscreenPlainSurface(width, height, format, pool, ppsurface, psharedhandle);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetRenderTarget(DWORD rendertargetindex, IDirect3DSurface9* prendertarget)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetRenderTarget(rendertargetindex, prendertarget);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetRenderTarget(DWORD rendertargetindex, IDirect3DSurface9** pprendertarget)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetRenderTarget(rendertargetindex, pprendertarget);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetDepthStencilSurface(IDirect3DSurface9* pnewzstencil)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetDepthStencilSurface(pnewzstencil);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetDepthStencilSurface(IDirect3DSurface9** ppzstencilsurface)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetDepthStencilSurface(ppzstencilsurface);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::BeginScene()
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->BeginScene();
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::EndScene()
{
  Validate();

  try
  {
    OnEndScene();
  }
  catch (std::exception const& e)
  {
    HADESMEM_DETAIL_TRACE_A(boost::diagnostic_information(e).c_str());
  }

  auto const proxy_call = MakeProxyCall(this);
  return device_->EndScene();
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::Clear(DWORD count, CONST D3DRECT* prects, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->Clear(count, prects, flags, color, z, stencil);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* pmatrix)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetTransform(state, pmatrix);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX* pmatrix)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetTransform(state, pmatrix);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::MultiplyTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* pmatrix)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->MultiplyTransform(state, pmatrix);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetViewport(CONST D3DVIEWPORT9* pviewport)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetViewport(pviewport);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetViewport(D3DVIEWPORT9* pviewport)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetViewport(pviewport);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetMaterial(CONST D3DMATERIAL9* pmaterial)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetMaterial(pmaterial);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetMaterial(D3DMATERIAL9* pmaterial)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetMaterial(pmaterial);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetLight(DWORD index, CONST D3DLIGHT9* light)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetLight(index, light);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetLight(DWORD index, D3DLIGHT9* light)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetLight(index, light);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::LightEnable(DWORD index, BOOL enable)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->LightEnable(index, enable);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetLightEnable(DWORD index, BOOL* penable)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetLightEnable(index, penable);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetClipPlane(DWORD index, CONST float* pplane)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetClipPlane(index, pplane);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetClipPlane(DWORD index, float* pplane)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetClipPlane(index, pplane);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetRenderState(D3DRENDERSTATETYPE state, DWORD value)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetRenderState(state, value);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetRenderState(D3DRENDERSTATETYPE state, DWORD* pvalue)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetRenderState(state, pvalue);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateStateBlock(D3DSTATEBLOCKTYPE type, IDirect3DStateBlock9** ppsb)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateStateBlock(type, ppsb);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::BeginStateBlock()
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->BeginStateBlock();
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::EndStateBlock(IDirect3DStateBlock9** ppsb)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->EndStateBlock(ppsb);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetClipStatus(CONST D3DCLIPSTATUS9* pclipstatus)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetClipStatus(pclipstatus);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetClipStatus(D3DCLIPSTATUS9* pclipstatus)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetClipStatus(pclipstatus);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetTexture(DWORD stage, IDirect3DBaseTexture9** pptexture)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetTexture(stage, pptexture);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetTexture(DWORD stage, IDirect3DBaseTexture9* ptexture)
{
  Validate();
  // TODO: Texture hooking support.
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetTexture(stage, ptexture);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD* pvalue)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetTextureStageState(stage, type, pvalue);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetTextureStageState(stage, type, value);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD* pvalue)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetSamplerState(sampler, type, pvalue);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetSamplerState(sampler, type, value);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::ValidateDevice(DWORD* pnumpasses)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->ValidateDevice(pnumpasses);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetPaletteEntries(UINT pallettenumber, CONST PALETTEENTRY* pentries)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetPaletteEntries(pallettenumber, pentries);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetPaletteEntries(UINT pallettenumber, PALETTEENTRY* pentries)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetPaletteEntries(pallettenumber, pentries);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetCurrentTexturePalette(UINT pallettenumber)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetCurrentTexturePalette(pallettenumber);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetCurrentTexturePalette(UINT* ppallettenumber)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetCurrentTexturePalette(ppallettenumber);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetScissorRect(CONST RECT* prect)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetScissorRect(prect);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetScissorRect(RECT* prect)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetScissorRect(prect);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetSoftwareVertexProcessing(BOOL bsoftware)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetSoftwareVertexProcessing(bsoftware);
}
  
BOOL __stdcall IDirect3DDevice9Proxy::GetSoftwareVertexProcessing()
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetSoftwareVertexProcessing();
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetNPatchMode(float nsegments)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetNPatchMode(nsegments);
}
  
float __stdcall IDirect3DDevice9Proxy::GetNPatchMode()
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetNPatchMode();
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::DrawPrimitive(D3DPRIMITIVETYPE primitivetype, UINT startvertex, UINT primitivecount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->DrawPrimitive(primitivetype, startvertex, primitivecount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::DrawIndexedPrimitive(D3DPRIMITIVETYPE primitivetype, INT basevertexindex, UINT minvertexindex, UINT numvertices, UINT startindex, UINT primcount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->DrawIndexedPrimitive(primitivetype, basevertexindex, minvertexindex, numvertices, startindex, primcount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::DrawPrimitiveUP(D3DPRIMITIVETYPE primitivetype, UINT primitivecount, CONST void* pvertexstreamzerodata, UINT vertexstreamzerostride)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->DrawPrimitiveUP(primitivetype, primitivecount, pvertexstreamzerodata, vertexstreamzerostride);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE primitivetype, UINT minvertexindex, UINT numvertices, UINT primitivecount, CONST void* pindexdata, D3DFORMAT indexdataformat, CONST void* pvertexstreamzerodata, UINT vertexstreamzerostride)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->DrawIndexedPrimitiveUP(primitivetype, minvertexindex, numvertices, primitivecount, pindexdata, indexdataformat, pvertexstreamzerodata, vertexstreamzerostride);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::ProcessVertices(UINT srcstartindex, UINT destindex, UINT vertexcount, IDirect3DVertexBuffer9* pdestbuffer, IDirect3DVertexDeclaration9* pvertexdecl, DWORD flags)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->ProcessVertices(srcstartindex, destindex, vertexcount, pdestbuffer, pvertexdecl, flags);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pvertexelements, IDirect3DVertexDeclaration9** ppdecl)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateVertexDeclaration(pvertexelements, ppdecl);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetVertexDeclaration(IDirect3DVertexDeclaration9* pdecl)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetVertexDeclaration(pdecl);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetVertexDeclaration(IDirect3DVertexDeclaration9** ppdecl)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetVertexDeclaration(ppdecl);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetFVF(DWORD fvf)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetFVF(fvf);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetFVF(DWORD* pfvf)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetFVF(pfvf);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateVertexShader(CONST DWORD* pfunction, IDirect3DVertexShader9** ppshader)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateVertexShader(pfunction, ppshader);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetVertexShader(IDirect3DVertexShader9* pshader)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetVertexShader(pshader);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetVertexShader(IDirect3DVertexShader9** ppshader)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetVertexShader(ppshader);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetVertexShaderConstantF(UINT startregister, CONST float* pconstantdata, UINT vector4fcount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetVertexShaderConstantF(startregister, pconstantdata, vector4fcount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetVertexShaderConstantF(UINT startregister, float* pconstantdata, UINT vector4fcount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetVertexShaderConstantF(startregister, pconstantdata, vector4fcount);
}

HRESULT __stdcall IDirect3DDevice9Proxy::SetVertexShaderConstantI(UINT startregister, CONST int* pconstantdata, UINT vector4icount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetVertexShaderConstantI(startregister, pconstantdata, vector4icount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetVertexShaderConstantI(UINT startregister, int* pconstantdata, UINT vector4icount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetVertexShaderConstantI(startregister, pconstantdata, vector4icount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetVertexShaderConstantB(UINT startregister, CONST BOOL* pconstantdata, UINT  boolcount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetVertexShaderConstantB(startregister, pconstantdata, boolcount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetVertexShaderConstantB(UINT startregister, BOOL* pconstantdata, UINT boolcount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetVertexShaderConstantB(startregister, pconstantdata, boolcount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetStreamSource(UINT streamnumber, IDirect3DVertexBuffer9* pstreamdata, UINT offsetinbytes, UINT stride)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetStreamSource(streamnumber, pstreamdata, offsetinbytes, stride);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetStreamSource(UINT streamnumber, IDirect3DVertexBuffer9** ppstreamdata, UINT* offsetinbytes, UINT* pstride)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetStreamSource(streamnumber, ppstreamdata, offsetinbytes, pstride);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetStreamSourceFreq(UINT streamnumber, UINT divider)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetStreamSourceFreq(streamnumber, divider);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetStreamSourceFreq(UINT streamnumber, UINT* divider)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetStreamSourceFreq(streamnumber, divider);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetIndices(IDirect3DIndexBuffer9* pindexdata)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetIndices(pindexdata);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetIndices(IDirect3DIndexBuffer9** ppindexdata)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetIndices(ppindexdata);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreatePixelShader(CONST DWORD* pfunction, IDirect3DPixelShader9** ppshader)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreatePixelShader(pfunction, ppshader);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetPixelShader(IDirect3DPixelShader9* pshader)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetPixelShader(pshader);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetPixelShader(IDirect3DPixelShader9** ppshader)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetPixelShader(ppshader);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetPixelShaderConstantF(UINT startregister, CONST float* pconstantdata, UINT vector4fcount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetPixelShaderConstantF(startregister, pconstantdata, vector4fcount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetPixelShaderConstantF(UINT startregister, float* pconstantdata, UINT vector4fcount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetPixelShaderConstantF(startregister, pconstantdata, vector4fcount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetPixelShaderConstantI(UINT startregister, CONST int* pconstantdata, UINT vector4icount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetPixelShaderConstantI(startregister, pconstantdata, vector4icount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetPixelShaderConstantI(UINT startregister, int* pconstantdata, UINT vector4icount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetPixelShaderConstantI(startregister, pconstantdata, vector4icount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::SetPixelShaderConstantB(UINT startregister, CONST BOOL* pconstantdata, UINT  boolcount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->SetPixelShaderConstantB(startregister, pconstantdata, boolcount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::GetPixelShaderConstantB(UINT startregister, BOOL* pconstantdata, UINT boolcount)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->GetPixelShaderConstantB(startregister, pconstantdata, boolcount);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::DrawRectPatch(UINT handle, CONST float* pnumsegs, CONST D3DRECTPATCH_INFO* prectpatchinfo)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->DrawRectPatch(handle, pnumsegs, prectpatchinfo);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::DrawTriPatch(UINT handle, CONST float* pnumsegs, CONST D3DTRIPATCH_INFO* ptripatchinfo)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->DrawTriPatch(handle, pnumsegs, ptripatchinfo);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::DeletePatch(UINT handle)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->DeletePatch(handle);
}
  
HRESULT __stdcall IDirect3DDevice9Proxy::CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9** ppquery)
{
  Validate();
  auto const proxy_call = MakeProxyCall(this);
  return device_->CreateQuery(type, ppquery);
}

void IDirect3DDevice9Proxy::PreCall()
{
  ++call_count_;
}

void IDirect3DDevice9Proxy::PostCall()
{
  --call_count_;
}

void IDirect3DDevice9Proxy::Validate() const
{
  HADESMEM_DETAIL_ASSERT(magic_ == 0xDEADBEEF);

  HADESMEM_DETAIL_ASSERT(device_ != nullptr);
}

IDirect3DStateBlock9* IDirect3DDevice9Proxy::CreateDefaultStateBlock()
{
  auto const proxy_call = MakeProxyCall(this);

  HRESULT const begin_state_block_hr = device_->BeginStateBlock();
  if (FAILED(begin_state_block_hr))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(hadesmem::Error() << 
      hadesmem::ErrorString("IDirect3DDevice9::BeginStateBlock failed.") << 
      hadesmem::ErrorCodeWinLast(last_error) << 
      hadesmem::ErrorCodeWinHr(begin_state_block_hr));
  }
  
  float const zero = 0.0f;
  float const one = 1.0f;
  float const sixty_four = 64.0f;
  union FloatToDword
  {
    float f;
    DWORD d;
  };
  FloatToDword conv;
  conv.f = zero;
  DWORD const zero_dword = conv.d;
  conv.f = one;
  DWORD const one_dword = conv.d;
  conv.f = sixty_four;
  DWORD const sixty_four_dword = conv.d;
  struct RenderState
  {
    D3DRENDERSTATETYPE first;
    DWORD second;
  };
  RenderState render_states[] = 
    {
      { D3DRS_ZENABLE, D3DZB_TRUE }, 
      { D3DRS_FILLMODE, D3DFILL_SOLID }, 
      { D3DRS_SHADEMODE, D3DSHADE_GOURAUD }, 
      { D3DRS_ZWRITEENABLE, TRUE }, 
      { D3DRS_ALPHATESTENABLE, FALSE }, 
      { D3DRS_LASTPIXEL, FALSE }, 
      { D3DRS_SRCBLEND, D3DBLEND_ONE }, 
      { D3DRS_DESTBLEND, D3DBLEND_ZERO }, 
      { D3DRS_CULLMODE, D3DCULL_CCW }, 
      { D3DRS_ZFUNC, D3DCMP_LESSEQUAL }, 
      { D3DRS_ALPHAREF, 0 }, 
      { D3DRS_ALPHAFUNC, D3DCMP_ALWAYS }, 
      { D3DRS_DITHERENABLE, FALSE }, 
      { D3DRS_ALPHABLENDENABLE, FALSE }, 
      { D3DRS_FOGENABLE, FALSE }, 
      { D3DRS_SPECULARENABLE, FALSE }, 
      { D3DRS_FOGCOLOR, 0 }, 
      { D3DRS_FOGTABLEMODE, D3DFOG_NONE }, 
      { D3DRS_FOGSTART, zero_dword }, 
      { D3DRS_FOGEND, one_dword }, 
      { D3DRS_FOGDENSITY, one_dword }, 
      { D3DRS_RANGEFOGENABLE, FALSE }, 
      { D3DRS_STENCILENABLE, FALSE }, 
      { D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP }, 
      { D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP }, 
      { D3DRS_STENCILPASS, D3DSTENCILOP_KEEP }, 
      { D3DRS_STENCILFUNC, D3DCMP_ALWAYS }, 
      { D3DRS_STENCILREF, 0 }, 
      { D3DRS_STENCILMASK, 0xFFFFFFFF }, 
      { D3DRS_STENCILWRITEMASK, 0xFFFFFFFF }, 
      { D3DRS_TEXTUREFACTOR, 0xFFFFFFFF }, 
      { D3DRS_WRAP0, 0 }, 
      { D3DRS_CLIPPING, TRUE }, 
      { D3DRS_LIGHTING, TRUE }, 
      { D3DRS_AMBIENT, 0 }, 
      { D3DRS_FOGVERTEXMODE, D3DFOG_NONE }, 
      { D3DRS_COLORVERTEX, TRUE }, 
      { D3DRS_LOCALVIEWER, TRUE }, 
      { D3DRS_NORMALIZENORMALS, FALSE }, 
      { D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1 }, 
      { D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2 }, 
      { D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL }, 
      { D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL }, 
      { D3DRS_VERTEXBLEND, D3DVBF_DISABLE }, 
      { D3DRS_CLIPPLANEENABLE, 0 }, 
      { D3DRS_POINTSIZE, one_dword }, 
      { D3DRS_POINTSIZE_MIN, one_dword }, 
      { D3DRS_POINTSPRITEENABLE, FALSE }, 
      { D3DRS_POINTSCALEENABLE, FALSE }, 
      { D3DRS_POINTSCALE_A, one_dword }, 
      { D3DRS_POINTSCALE_B, zero_dword }, 
      { D3DRS_POINTSCALE_C, zero_dword }, 
      { D3DRS_MULTISAMPLEANTIALIAS, TRUE }, 
      { D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF }, 
      { D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE }, 
      { D3DRS_DEBUGMONITORTOKEN, D3DDMT_ENABLE }, 
      { D3DRS_POINTSIZE_MAX, sixty_four_dword }, 
      { D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE }, 
      { D3DRS_COLORWRITEENABLE, 0x0000000F }, 
      { D3DRS_TWEENFACTOR, zero_dword }, 
      { D3DRS_BLENDOP, D3DBLENDOP_ADD }, 
      { D3DRS_BLENDOP, D3DBLENDOP_ADD }, 
      { D3DRS_POSITIONDEGREE, D3DDEGREE_CUBIC }, 
      { D3DRS_NORMALDEGREE, D3DDEGREE_LINEAR }, 
      { D3DRS_SCISSORTESTENABLE, FALSE }, 
      { D3DRS_SLOPESCALEDEPTHBIAS, 0 }, 
      { D3DRS_ANTIALIASEDLINEENABLE, FALSE }, 
      { D3DRS_MINTESSELLATIONLEVEL, one_dword }, 
      { D3DRS_MAXTESSELLATIONLEVEL, one_dword }, 
      { D3DRS_ADAPTIVETESS_X, zero_dword }, 
      { D3DRS_ADAPTIVETESS_Y, zero_dword }, 
      { D3DRS_ADAPTIVETESS_Z, one_dword }, 
      { D3DRS_ADAPTIVETESS_W, zero_dword }, 
      { D3DRS_TWOSIDEDSTENCILMODE, FALSE }, 
      { D3DRS_CCW_STENCILFAIL, 0x00000001 }, 
      { D3DRS_CCW_STENCILZFAIL, 0x00000001 }, 
      { D3DRS_CCW_STENCILPASS, 0x00000001 }, 
      { D3DRS_CCW_STENCILFUNC, 0x00000008 }, 
      { D3DRS_COLORWRITEENABLE1, 0x0000000f }, 
      { D3DRS_COLORWRITEENABLE2, 0x0000000f }, 
      { D3DRS_COLORWRITEENABLE3, 0x0000000f }, 
      { D3DRS_BLENDFACTOR, 0xffffffff }, 
      { D3DRS_SRGBWRITEENABLE, 0 }, 
      { D3DRS_DEPTHBIAS, 0 }, 
      { D3DRS_SEPARATEALPHABLENDENABLE, FALSE }, 
      { D3DRS_SRCBLENDALPHA, D3DBLEND_ONE }, 
      { D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO }, 
      { D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD }
    };
  for (auto const& render_state : render_states)
  {
    HRESULT const set_render_state_hr = device_->SetRenderState(
      render_state.first, render_state.second);
    if (FAILED(set_render_state_hr))
    {
      DWORD const last_error = ::GetLastError();
      BOOST_THROW_EXCEPTION(hadesmem::Error() << 
        hadesmem::ErrorString("IDirect3DDevice9::SetPixelShader failed.") << 
        hadesmem::ErrorCodeWinLast(last_error) << 
        hadesmem::ErrorCodeWinHr(set_render_state_hr));
    }
  }
  
  HRESULT const set_pixel_shader_hr = device_->SetPixelShader(nullptr);
  if (FAILED(set_pixel_shader_hr))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(hadesmem::Error() << 
      hadesmem::ErrorString("IDirect3DDevice9::SetPixelShader failed.") << 
      hadesmem::ErrorCodeWinLast(last_error) << 
      hadesmem::ErrorCodeWinHr(set_pixel_shader_hr));
  }

  HRESULT const set_vertex_shader_hr = device_->SetVertexShader(nullptr);
  if (FAILED(set_vertex_shader_hr))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(hadesmem::Error() << 
      hadesmem::ErrorString("IDirect3DDevice9::SetVertexShader failed.") << 
      hadesmem::ErrorCodeWinLast(last_error) << 
      hadesmem::ErrorCodeWinHr(set_vertex_shader_hr));
  }
  
  HRESULT const set_texture_hr = device_->SetTexture(0, nullptr);
  if (FAILED(set_texture_hr))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(hadesmem::Error() << 
      hadesmem::ErrorString("IDirect3DDevice9::SetTexture failed.") << 
      hadesmem::ErrorCodeWinLast(last_error) << 
      hadesmem::ErrorCodeWinHr(set_texture_hr));
  }

  HRESULT const set_vertex_declaration_hr = 
    device_->SetVertexDeclaration(nullptr);
  if (FAILED(set_vertex_declaration_hr))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(hadesmem::Error() << 
      hadesmem::ErrorString("IDirect3DDevice9::SetVertexDeclaration failed.") << 
      hadesmem::ErrorCodeWinLast(last_error) << 
      hadesmem::ErrorCodeWinHr(set_vertex_declaration_hr));
  }

  IDirect3DStateBlock9* state_block = nullptr;
  HRESULT const end_state_block_hr = device_->EndStateBlock(&state_block);
  if (FAILED(end_state_block_hr))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(hadesmem::Error() << 
      hadesmem::ErrorString("IDirect3DDevice9::EndStateBlock failed.") << 
      hadesmem::ErrorCodeWinLast(last_error) << 
      hadesmem::ErrorCodeWinHr(end_state_block_hr));
  }

  return state_block;
}

void IDirect3DDevice9Proxy::OnEndScene()
{
  auto const proxy_call = MakeProxyCall(this);

  if (render_data_.state_block)
  {
    if (FAILED(render_data_.state_block->Capture()))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! IDirect3DStateBlock9::Capture failed "
        "(state block).");
      return;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnEndScene called with invalid render "
      "data (state block).");
  }

  if (render_data_.default_state_block)
  {
    if (FAILED(render_data_.default_state_block->Apply()))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! IDirect3DStateBlock9::Apply failed "
        "(default state block).");
      return;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnEndScene called with invalid render "
      "data (default state block).");
  }

  auto const draw_line = [&] (D3DXVECTOR2 start, D3DXVECTOR2 end, 
    float width, D3DCOLOR colour) -> bool
  {
    if (FAILED(render_data_.line->SetWidth(width)))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXLine::SetWidth failed.");
      return false;
    }

    if (FAILED(render_data_.line->Begin()))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXLine::Begin failed.");
      return false;
    }
    
    D3DXVECTOR2 vec[2] = { start, end };
    if (FAILED(render_data_.line->Draw(vec, 2, colour)))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXLine::Draw failed.");
      render_data_.line->End();
      return false;
    }
      
    if (FAILED(render_data_.line->End()))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXLine::End failed.");
      return false;
    }

    return true;
  };

  auto const draw_box = [&] (D3DXVECTOR2 bottom_left, D3DXVECTOR2 top_right, 
    float line_width, D3DCOLOR colour) -> bool
  {
    float width = top_right[0] - bottom_left[0];
    float height = top_right[1] - bottom_left[1];

    D3DXVECTOR2 top_left(top_right[0] - width, top_right[1]);
    D3DXVECTOR2 bottom_right(top_right[0], top_right[1] - height);

    if (!draw_line(bottom_left, top_left, line_width, colour) 
      || !draw_line(bottom_left, bottom_right, line_width, colour) 
      || !draw_line(bottom_right, top_right, line_width, colour) 
      || !draw_line(top_left, top_right, line_width, colour))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! DrawLine failed.");
      return false;
    }

    return true;
  };

  D3DVIEWPORT9 viewport;
  if (FAILED(device_->GetViewport(&viewport)))
  {
    HADESMEM_DETAIL_TRACE_A("Warning! IDirect3DDevice9::GetViewport failed.");
    return;
  }

  if (render_data_.line)
  {
    float const line_width = 3.0f;
    D3DXVECTOR2 const top_left(1,1);
    D3DXVECTOR2 const bottom_right(
      static_cast<float>(viewport.Width) - line_width + 1, 
      static_cast<float>(viewport.Height) - line_width + 1);
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-sign-overflow"
#endif // #if defined(HADESMEM_CLANG)
    if (!draw_box(top_left, bottom_right, line_width, 
      D3DCOLOR_ARGB(255, 0, 255, 0)))
    {
      return;
    }
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnEndScene called with invalid render "
      "data (line).");
    return;
  }
  
  if (render_data_.font)
  {
    std::wstring const text(L"Hades");
    LONG const text_draw_x = 10;
    LONG const text_draw_y = 10;
    RECT text_draw_rect = { text_draw_x, text_draw_y, 0, 0 };
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-sign-overflow"
#endif // #if defined(HADESMEM_CLANG)
    if (FAILED(render_data_.font->DrawText(
      nullptr, 
      text.c_str(), 
      -1, 
      &text_draw_rect, 
      DT_NOCLIP, 
      D3DCOLOR_RGBA(0, 255, 0, 255))))
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXFont::DrawText failed.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnEndScene called with invalid render "
      "data (font).");
  }

  if (render_data_.state_block)
  {
    if (FAILED(render_data_.state_block->Apply()))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! IDirect3DStateBlock9::Apply failed "
        "(default state block).");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnEndScene called with invalid render "
      "data (default state block).");
  }
}

void IDirect3DDevice9Proxy::OnPreReset()
{
  OutputDebugStringA("OnPreReset called for device.");
  
  HADESMEM_DETAIL_ASSERT(call_count_ == 0);

  if (render_data_.state_block)
  {
    if (render_data_.state_block->Release())
    {
      HADESMEM_DETAIL_TRACE_A("Warning! IDirect3DStateBlock9 leaked in "
        "OnPreReset.");
    }
    render_data_.state_block = nullptr;
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnPreReset called with invalid render "
      "data (state block).");
  }

  if (render_data_.default_state_block)
  {
    if (render_data_.default_state_block->Release())
    {
      HADESMEM_DETAIL_TRACE_A("Warning! IDirect3DStateBlock9 leaked in "
        "OnPreReset (default).");
    }
    render_data_.default_state_block = nullptr;
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnPreReset called with invalid render data"
      " (default state block).");
  }

  if (render_data_.line)
  {
    if (FAILED(render_data_.line->OnLostDevice()))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXLine::OnLostDevice failed.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnPreReset called with invalid render "
      "data (line).");
  }

  if (render_data_.font)
  {
    if (FAILED(render_data_.font->OnLostDevice()))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXFont::OnLostDevice failed.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnPreReset called with invalid render "
      "data (font).");
  }
}

void IDirect3DDevice9Proxy::OnPostReset()
{
  OutputDebugStringA("OnPostReset called for device.");
  
  HADESMEM_DETAIL_ASSERT(render_data_.state_block == nullptr);

  if (render_data_.state_block)
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnPostReset called with invalid render "
      "data (state block).");
  }
  else
  {
    if (FAILED(device_->CreateStateBlock(D3DSBT_ALL, &render_data_.state_block)))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! IDirect3DDevice9::CreateStateBlock "
        "failed.");
    }
  }
  
  HADESMEM_DETAIL_ASSERT(render_data_.default_state_block == nullptr);

  if (render_data_.default_state_block)
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnPostReset called with invalid render "
      "data (state block).");
  }
  else
  {
    render_data_.default_state_block = CreateDefaultStateBlock();
    if (!render_data_.default_state_block)
    {
      HADESMEM_DETAIL_TRACE_A("Warning! CreateDefaultStateBlock failed "
        "(default).");
    }
  }
  
  HADESMEM_DETAIL_ASSERT(render_data_.line != nullptr);

  if (render_data_.line)
  {
    if (FAILED(render_data_.line->OnResetDevice()))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXLine::OnResetDevice failed.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnPostReset called with invalid render "
      "data (line).");
  }
  
  HADESMEM_DETAIL_ASSERT(render_data_.font != nullptr);

  if (render_data_.font)
  {
    if (FAILED(render_data_.font->OnResetDevice()))
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXFont::OnResetDevice failed.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! OnPostReset called with invalid render "
      "data (font).");
  }
}

// TODO: Add checks to ensure that 
void IDirect3DDevice9Proxy::OnRelease()
{
  HADESMEM_DETAIL_TRACE_A("OnRelease actually cleaning up!");

  HADESMEM_DETAIL_ASSERT(call_count_ == 0);

  HADESMEM_DETAIL_ASSERT(render_data_.state_block != nullptr);

  if (render_data_.state_block)
  {
    if (render_data_.state_block->Release())
    {
      HADESMEM_DETAIL_TRACE_A("Warning! IDirect3DStateBlock9 leaked in "
        "Release.");
    }
    render_data_.state_block = nullptr;

    HADESMEM_DETAIL_TRACE_A("Cleaned up IDirect3DStateBlock9.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! Release called with invalid render data "
      "(state block).");
  }
  
  HADESMEM_DETAIL_ASSERT(render_data_.default_state_block != nullptr);

  if (render_data_.default_state_block)
  {
    if (render_data_.default_state_block->Release())
    {
      HADESMEM_DETAIL_TRACE_A("Warning! IDirect3DStateBlock9 leaked in "
        "Release (default).");
    }
    render_data_.default_state_block = nullptr;

    HADESMEM_DETAIL_TRACE_A("Cleaned up IDirect3DStateBlock9 (default).");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! Release called with invalid render "
      "data (default state block).");
  }
  
  HADESMEM_DETAIL_ASSERT(render_data_.line != nullptr);

  if (render_data_.line)
  {
    if (render_data_.line->Release())
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXLine leaked in Release "
        "(default).");
    }
    render_data_.line = nullptr;

    HADESMEM_DETAIL_TRACE_A("Cleaned up ID3DXLine.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! Release called with invalid render "
      "data (line).");
  }
  
  HADESMEM_DETAIL_ASSERT(render_data_.font != nullptr);

  if (render_data_.font)
  {
    if (render_data_.font->Release())
    {
      HADESMEM_DETAIL_TRACE_A("Warning! ID3DXFont leaked in Release "
        "(default).");
    }
    render_data_.font = nullptr;

    HADESMEM_DETAIL_TRACE_A("Cleaned up ID3DXFont.");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Warning! Release called with invalid render "
      "data (line).");
  }
}
