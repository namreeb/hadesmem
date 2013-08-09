// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/atomic.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

// Work around a Clang issue.
// c:/mingw32-dw2/bin/../lib/clang/3.2/../../../i686-w64-mingw32/include
// \d3dx9math.inl:994:15: error: use of undeclared identifier 'max'; did 
// you mean 'fmax'?
// TODO: Figure out whether this is the fault of Clang, or MinGW, or 
// something else.
#include <hadesmem/config.hpp>
#if defined(HADESMEM_CLANG)
using std::min;
using std::max;
#endif // #if defined(HADESMEM_CLANG)

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <d3d9.h>
#include <d3dx9.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/smart_handle.hpp>

class IDirect3DDevice9Proxy : public IDirect3DDevice9
{
public:
  explicit IDirect3DDevice9Proxy(IDirect3DDevice9* ppdevice);

  virtual ~IDirect3DDevice9Proxy();

  // IUnknown methods
  virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvobj);
  
  virtual ULONG __stdcall AddRef();
  
  virtual ULONG __stdcall Release();
  
  // IDirect3DDevice9 methods
  virtual HRESULT __stdcall TestCooperativeLevel();
  
  virtual UINT __stdcall GetAvailableTextureMem();
  
  virtual HRESULT __stdcall EvictManagedResources();
  
  virtual HRESULT __stdcall GetDirect3D(IDirect3D9** ppd3d9);
  
  virtual HRESULT __stdcall GetDeviceCaps(D3DCAPS9* pcaps);
  
  virtual HRESULT __stdcall GetDisplayMode(UINT iswapchain, D3DDISPLAYMODE* pmode);
  
  virtual HRESULT __stdcall GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pparameters);
  
  virtual HRESULT __stdcall SetCursorProperties(UINT x_hot_spot, UINT y_hot_spot, IDirect3DSurface9* pcursorbitmap);
  
  virtual void __stdcall SetCursorPosition(int x, int y, DWORD flags);
  
  virtual BOOL __stdcall ShowCursor(BOOL bshow);
  
  virtual HRESULT __stdcall CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* ppresentationparameters, IDirect3DSwapChain9** pswapchain);
  
  virtual HRESULT __stdcall GetSwapChain(UINT iswapchain, IDirect3DSwapChain9** pswapchain);
  
  virtual UINT __stdcall GetNumberOfSwapChains();
  
  virtual HRESULT __stdcall Reset(D3DPRESENT_PARAMETERS* pPresentationParameters);
  
  virtual HRESULT __stdcall Present(CONST RECT* psourcerect, CONST RECT* pdestrect, HWND hdestwindowoverride, CONST RGNDATA* pdirtyregion);
  
  virtual HRESULT __stdcall GetBackBuffer(UINT iswapchain, UINT ibackbuffer, D3DBACKBUFFER_TYPE type, IDirect3DSurface9** ppbackbuffer);
  
  virtual HRESULT __stdcall GetRasterStatus(UINT iswapchain, D3DRASTER_STATUS* prasterstatus);
  
  virtual HRESULT __stdcall SetDialogBoxMode(BOOL benabledialogs);
  
  virtual void __stdcall SetGammaRamp(UINT iswapchain, DWORD flags, CONST D3DGAMMARAMP* pramp);
  
  virtual void __stdcall GetGammaRamp(UINT iswapchain, D3DGAMMARAMP* pramp);
  
  virtual HRESULT __stdcall CreateTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture9** pptexture, HANDLE* psharedhandle);
  
  virtual HRESULT __stdcall CreateVolumeTexture(UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DVolumeTexture9** ppvolumetexture, HANDLE* psharedhandle);
  
  virtual HRESULT __stdcall CreateCubeTexture(UINT edgelength, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture9** ppcubetexture, HANDLE* psharedhandle);
  
  virtual HRESULT __stdcall CreateVertexBuffer(UINT Length, DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer9** ppvertexbuffer, HANDLE* psharedhandle);
  
  virtual HRESULT __stdcall CreateIndexBuffer(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer9** ppindexbuffer, HANDLE* psharedhandle);
  
  virtual HRESULT __stdcall CreateRenderTarget(UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisamplequality, BOOL lockable, IDirect3DSurface9** ppsurface, HANDLE* psharedhandle);
  
  virtual HRESULT __stdcall CreateDepthStencilSurface(UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample, DWORD multisamplequality, BOOL discard, IDirect3DSurface9** ppsurface, HANDLE* psharedhandle);
  
  virtual HRESULT __stdcall UpdateSurface(IDirect3DSurface9* psourcesurface, CONST RECT* psourcerect, IDirect3DSurface9* pdestinationsurface, CONST POINT* pdestpoint);
  
  virtual HRESULT __stdcall UpdateTexture(IDirect3DBaseTexture9* psourcetexture, IDirect3DBaseTexture9* pdestinationtexture);
  
  virtual HRESULT __stdcall GetRenderTargetData(IDirect3DSurface9* prendertarget, IDirect3DSurface9* pdestsurface);
  
  virtual HRESULT __stdcall GetFrontBufferData(UINT iswapchain, IDirect3DSurface9* pdestsurface);
  
  virtual HRESULT __stdcall StretchRect(IDirect3DSurface9* psourcesurface, CONST RECT* psourcerect, IDirect3DSurface9* pdestsurface, CONST RECT* pdestrect, D3DTEXTUREFILTERTYPE filter);
  
  virtual HRESULT __stdcall ColorFill(IDirect3DSurface9* psurface, CONST RECT* prect, D3DCOLOR color);
  
  virtual HRESULT __stdcall CreateOffscreenPlainSurface(UINT width, UINT height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9** ppsurface, HANDLE* psharedhandle);
  
  virtual HRESULT __stdcall SetRenderTarget(DWORD rendertargetindex, IDirect3DSurface9* prendertarget);
  
  virtual HRESULT __stdcall GetRenderTarget(DWORD rendertargetindex, IDirect3DSurface9** pprendertarget);
  
  virtual HRESULT __stdcall SetDepthStencilSurface(IDirect3DSurface9* pnewzstencil);
  
  virtual HRESULT __stdcall GetDepthStencilSurface(IDirect3DSurface9** ppzstencilsurface);
  
  virtual HRESULT __stdcall BeginScene();
  
  virtual HRESULT __stdcall EndScene();
  
  virtual HRESULT __stdcall Clear(DWORD count, CONST D3DRECT* prects, DWORD flags, D3DCOLOR color, float z, DWORD stencil);
  
  virtual HRESULT __stdcall SetTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* pmatrix);
  
  virtual HRESULT __stdcall GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX* pmatrix);
  
  virtual HRESULT __stdcall MultiplyTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* pmatrix);
  
  virtual HRESULT __stdcall SetViewport(CONST D3DVIEWPORT9* pviewport);
  
  virtual HRESULT __stdcall GetViewport(D3DVIEWPORT9* pviewport);
  
  virtual HRESULT __stdcall SetMaterial(CONST D3DMATERIAL9* pmaterial);
  
  virtual HRESULT __stdcall GetMaterial(D3DMATERIAL9* pmaterial);
  
  virtual HRESULT __stdcall SetLight(DWORD index, CONST D3DLIGHT9* light);
  
  virtual HRESULT __stdcall GetLight(DWORD index, D3DLIGHT9* light);
  
  virtual HRESULT __stdcall LightEnable(DWORD index, BOOL enable);
  
  virtual HRESULT __stdcall GetLightEnable(DWORD index, BOOL* penable);
  
  virtual HRESULT __stdcall SetClipPlane(DWORD index, CONST float* pplane);
  
  virtual HRESULT __stdcall GetClipPlane(DWORD index, float* pplane);
  
  virtual HRESULT __stdcall SetRenderState(D3DRENDERSTATETYPE state, DWORD value);
  
  virtual HRESULT __stdcall GetRenderState(D3DRENDERSTATETYPE state, DWORD* pvalue);
  
  virtual HRESULT __stdcall CreateStateBlock(D3DSTATEBLOCKTYPE type, IDirect3DStateBlock9** ppsb);
  
  virtual HRESULT __stdcall BeginStateBlock();
  
  virtual HRESULT __stdcall EndStateBlock(IDirect3DStateBlock9** ppsb);
  
  virtual HRESULT __stdcall SetClipStatus(CONST D3DCLIPSTATUS9* pclipstatus);
  
  virtual HRESULT __stdcall GetClipStatus(D3DCLIPSTATUS9* pclipstatus);
  
  virtual HRESULT __stdcall GetTexture(DWORD stage, IDirect3DBaseTexture9** pptexture);
  
  virtual HRESULT __stdcall SetTexture(DWORD stage, IDirect3DBaseTexture9* ptexture);
  
  virtual HRESULT __stdcall GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD* pvalue);
  
  virtual HRESULT __stdcall SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value);
  
  virtual HRESULT __stdcall GetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD* pvalue);
  
  virtual HRESULT __stdcall SetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value);
  
  virtual HRESULT __stdcall ValidateDevice(DWORD* pnumpasses);
  
  virtual HRESULT __stdcall SetPaletteEntries(UINT pallettenumber, CONST PALETTEENTRY* pentries);
  
  virtual HRESULT __stdcall GetPaletteEntries(UINT pallettenumber, PALETTEENTRY* pentries);
  
  virtual HRESULT __stdcall SetCurrentTexturePalette(UINT pallettenumber);
  
  virtual HRESULT __stdcall GetCurrentTexturePalette(UINT* ppallettenumber);
  
  virtual HRESULT __stdcall SetScissorRect(CONST RECT* prect);
  
  virtual HRESULT __stdcall GetScissorRect(RECT* prect);
  
  virtual HRESULT __stdcall SetSoftwareVertexProcessing(BOOL bsoftware);
  
  virtual BOOL __stdcall GetSoftwareVertexProcessing();
  
  virtual HRESULT __stdcall SetNPatchMode(float nsegments);
  
  virtual float __stdcall GetNPatchMode();
  
  virtual HRESULT __stdcall DrawPrimitive(D3DPRIMITIVETYPE primitivetype, UINT startvertex, UINT primitivecount);
  
  virtual HRESULT __stdcall DrawIndexedPrimitive(D3DPRIMITIVETYPE primitivetype, INT basevertexindex, UINT minvertexindex, UINT numvertices, UINT startindex, UINT primcount);
  
  virtual HRESULT __stdcall DrawPrimitiveUP(D3DPRIMITIVETYPE primitivetype, UINT primitivecount, CONST void* pvertexstreamzerodata, UINT vertexstreamzerostride);
  
  virtual HRESULT __stdcall DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE primitivetype, UINT minvertexindex, UINT numvertices, UINT primitivecount, CONST void* pindexdata, D3DFORMAT indexdataformat, CONST void* pvertexstreamzerodata, UINT vertexstreamzerostride);
  
  virtual HRESULT __stdcall ProcessVertices(UINT srcstartindex, UINT destindex, UINT vertexcount, IDirect3DVertexBuffer9* pdestbuffer, IDirect3DVertexDeclaration9* pvertexdecl, DWORD flags);
  
  virtual HRESULT __stdcall CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pvertexelements, IDirect3DVertexDeclaration9** ppdecl);
  
  virtual HRESULT __stdcall SetVertexDeclaration(IDirect3DVertexDeclaration9* pdecl);
  
  virtual HRESULT __stdcall GetVertexDeclaration(IDirect3DVertexDeclaration9** ppdecl);
  
  virtual HRESULT __stdcall SetFVF(DWORD fvf);
  
  virtual HRESULT __stdcall GetFVF(DWORD* pfvf);
  
  virtual HRESULT __stdcall CreateVertexShader(CONST DWORD* pfunction, IDirect3DVertexShader9** ppshader);
  
  virtual HRESULT __stdcall SetVertexShader(IDirect3DVertexShader9* pshader);
  
  virtual HRESULT __stdcall GetVertexShader(IDirect3DVertexShader9** ppshader);
  
  virtual HRESULT __stdcall SetVertexShaderConstantF(UINT startregister, CONST float* pconstantdata, UINT vector4fcount);
  
  virtual HRESULT __stdcall GetVertexShaderConstantF(UINT startregister, float* pconstantdata, UINT vector4fcount);

  virtual HRESULT __stdcall SetVertexShaderConstantI(UINT startregister, CONST int* pconstantdata, UINT vector4icount);
  
  virtual HRESULT __stdcall GetVertexShaderConstantI(UINT startregister, int* pconstantdata, UINT vector4icount);
  
  virtual HRESULT __stdcall SetVertexShaderConstantB(UINT startregister, CONST BOOL* pconstantdata, UINT  boolcount);
  
  virtual HRESULT __stdcall GetVertexShaderConstantB(UINT startregister, BOOL* pconstantdata, UINT boolcount);
  
  virtual HRESULT __stdcall SetStreamSource(UINT streamnumber, IDirect3DVertexBuffer9* pstreamdata, UINT offsetinbytes, UINT stride);
  
  virtual HRESULT __stdcall GetStreamSource(UINT streamnumber, IDirect3DVertexBuffer9** ppstreamdata, UINT* offsetinbytes, UINT* pstride);
  
  virtual HRESULT __stdcall SetStreamSourceFreq(UINT streamnumber, UINT divider);
  
  virtual HRESULT __stdcall GetStreamSourceFreq(UINT streamnumber, UINT* divider);
  
  virtual HRESULT __stdcall SetIndices(IDirect3DIndexBuffer9* pindexdata);
  
  virtual HRESULT __stdcall GetIndices(IDirect3DIndexBuffer9** ppindexdata);
  
  virtual HRESULT __stdcall CreatePixelShader(CONST DWORD* pfunction, IDirect3DPixelShader9** ppshader);
  
  virtual HRESULT __stdcall SetPixelShader(IDirect3DPixelShader9* pshader);
  
  virtual HRESULT __stdcall GetPixelShader(IDirect3DPixelShader9** ppshader);
  
  virtual HRESULT __stdcall SetPixelShaderConstantF(UINT startregister, CONST float* pconstantdata, UINT vector4fcount);
  
  virtual HRESULT __stdcall GetPixelShaderConstantF(UINT startregister, float* pconstantdata, UINT vector4fcount);
  
  virtual HRESULT __stdcall SetPixelShaderConstantI(UINT startregister, CONST int* pconstantdata, UINT vector4icount);
  
  virtual HRESULT __stdcall GetPixelShaderConstantI(UINT startregister, int* pconstantdata, UINT vector4icount);
  
  virtual HRESULT __stdcall SetPixelShaderConstantB(UINT startregister, CONST BOOL* pconstantdata, UINT  boolcount);
  
  virtual HRESULT __stdcall GetPixelShaderConstantB(UINT startregister, BOOL* pconstantdata, UINT boolcount);
  
  virtual HRESULT __stdcall DrawRectPatch(UINT handle, CONST float* pnumsegs, CONST D3DRECTPATCH_INFO* prectpatchinfo);
  
  virtual HRESULT __stdcall DrawTriPatch(UINT handle, CONST float* pnumsegs, CONST D3DTRIPATCH_INFO* ptripatchinfo);
  
  virtual HRESULT __stdcall DeletePatch(UINT handle);
  
  virtual HRESULT __stdcall CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9** ppquery);
  
  void PreCall();

  void PostCall();

private:
  void Validate() const;

  IDirect3DStateBlock9* CreateDefaultStateBlock();

  void OnEndScene();

  void OnPreReset();

  void OnPostReset();

  void OnRelease();

  IDirect3DDevice9* device_;
  
  typedef std::unique_ptr<hadesmem::detail::SmartModuleHandle> 
    SmartModuleHandlePtr;

  SmartModuleHandlePtr d3dx_mod_;

  struct RenderData
  {
    HWND wnd;
    IDirect3DStateBlock9* state_block;
    IDirect3DStateBlock9* default_state_block;
    ID3DXLine* line;
    ID3DXFont* font;
  };
  RenderData render_data_;

  boost::atomic<ULONG> ref_count_;

  DWORD magic_;

  boost::atomic<ULONG> call_count_;
};
