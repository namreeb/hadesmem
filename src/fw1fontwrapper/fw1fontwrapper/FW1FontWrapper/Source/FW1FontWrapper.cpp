// FW1FontWrapper.cpp

#include "FW1Precompiled.h"

#include "CFW1Factory.h"

#ifndef FW1_DELAYLOAD_DWRITE_DLL
	#pragma comment (lib, "DWrite.lib")
#endif

#ifndef FW1_DELAYLOAD_D3DCOMPILER_XX_DLL
	#pragma comment (lib, "DWrite.lib")
#endif

#ifdef FW1_COMPILETODLL
	#ifndef _M_X64
		#pragma comment (linker, "/EXPORT:FW1CreateFactory=_FW1CreateFactory@8,@1")
	#endif
#endif

#ifdef __MINGW32__

#define UUID_DECL(type,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)                 \
  extern "C++" {                                                        \
  static const IID uuid_##type = {l,w1,w2, {b1,b2,b3,b4,b5,b6,b7,b8}}; \
  template<> const GUID &__mingw_uuidof<type>() {                     \
  return uuid_##type;                                               \
}                                                                   \
  template<> const GUID &__mingw_uuidof<type*>() {                    \
  return __mingw_uuidof<type>();                                    \
}                                                                   \
}

UUID_DECL(IFW1Object, 0x8D3C3FB1, 0xF2CC, 0x4331, 0xA6, 0x23, 0x03, 0x1F, 0x74, 0xC0, 0x66, 0x17);
UUID_DECL(IFW1GlyphSheet, 0x60CAB266, 0xC805, 0x461d, 0x82, 0xC0, 0x39, 0x24, 0x72, 0xEE, 0xCE, 0xFA);
UUID_DECL(IFW1GlyphAtlas, 0xA31EB6A2, 0x7458, 0x4E24, 0x82, 0xB3, 0x94, 0x4A, 0x95, 0x62, 0x3B, 0x1F);
UUID_DECL(IFW1GlyphProvider, 0xF8360043, 0x329D, 0x4EC9, 0xB0, 0xF8, 0xAC, 0xB0, 0x0F, 0xA7, 0x74, 0x20);
UUID_DECL(IFW1DWriteRenderTarget, 0xA1EB4141, 0x9A66, 0x4097, 0xA5, 0xB0, 0x6F, 0xC8, 0x4F, 0x8B, 0x16, 0x2C);
UUID_DECL(IFW1ColorRGBA, 0xA0EA03A0, 0x441D, 0x49BE, 0x9D, 0x2C, 0x4A, 0xE2, 0x7B, 0xB7, 0xA3, 0x27);
UUID_DECL(IFW1TextGeometry, 0x51E05736, 0x6AFF, 0x44A8, 0x97, 0x45, 0x77, 0x70, 0x5C, 0x99, 0xE8, 0xF2);
UUID_DECL(IFW1TextRenderer, 0x51E05736, 0x6AFF, 0x44A8, 0x97, 0x45, 0x77, 0x70, 0x5C, 0x99, 0xE8, 0xF2);
UUID_DECL(IFW1GlyphRenderStates, 0x906928B6, 0x79D8, 0x4B42, 0x8C, 0xE4, 0xDC, 0x7D, 0x70, 0x46, 0xF2, 0x06);
UUID_DECL(IFW1GlyphVertexDrawer, 0xE6CD7A32, 0x5B59, 0x463C, 0x9B, 0x1B, 0xD4, 0x40, 0x74, 0xFF, 0x65, 0x5B);
UUID_DECL(IFW1FontWrapper, 0x83347A5C, 0xB0B1, 0x460E, 0xA3, 0x5C, 0x42, 0x7E, 0x8B, 0x85, 0xF9, 0xF4);
UUID_DECL(IFW1Factory, 0x8004DB2B, 0xB5F9, 0x4420, 0xA6, 0xA2, 0xE1, 0x7E, 0x15, 0xE4, 0xC3, 0x36);

#endif


// Create FW1 factory
extern "C" HRESULT STDMETHODCALLTYPE FW1CreateFactory(UINT32 Version, IFW1Factory **ppFactory) {
	if(Version != FW1_VERSION)
		return E_FAIL;
	
	if(ppFactory == NULL)
		return E_INVALIDARG;
	
	FW1FontWrapper::CFW1Factory *pFactory = new FW1FontWrapper::CFW1Factory;
	HRESULT hResult = pFactory->initFactory();
	if(FAILED(hResult)) {
		pFactory->Release();
	}
	else {
		*ppFactory = pFactory;
		
		hResult = S_OK;
	}
	
	return hResult;
}
