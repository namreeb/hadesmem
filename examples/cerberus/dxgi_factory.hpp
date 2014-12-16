// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>

#include <windows.h>

#include <dxgi1_2.h>
#include <dxgi.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif // #if defined(HADESMEM_GCC)

class DXGIFactoryProxy : public IDXGIFactory2
{
public:
  explicit DXGIFactoryProxy(IDXGIFactory* factory) : factory_{factory}
  {
  }

  // IUnknown
  virtual HRESULT WINAPI QueryInterface(REFIID riid, void** obj) override;
  virtual ULONG WINAPI AddRef() override;
  virtual ULONG WINAPI Release() override;

  // IDXGIObject
  virtual HRESULT WINAPI SetPrivateData(_In_ REFGUID name,
                                        UINT data_size,
                                        _In_reads_bytes_(data_size)
                                          const void* data) override;
  virtual HRESULT WINAPI
    SetPrivateDataInterface(_In_ REFGUID name,
                            _In_ const IUnknown* unknown) override;
  virtual HRESULT WINAPI
    GetPrivateData(_In_ REFGUID name,
                   _Inout_ UINT* data_size,
                   _Out_writes_bytes_(*data_size) void* data) override;
  virtual HRESULT WINAPI
    GetParent(_In_ REFIID riid, _Out_ void** parent) override;

  // IDXGIFactory
  virtual HRESULT WINAPI
    EnumAdapters(UINT adapter_index, _Out_ IDXGIAdapter** adapter) override;
  virtual HRESULT WINAPI
    MakeWindowAssociation(HWND window_handle, UINT flags) override;
  virtual HRESULT WINAPI
    GetWindowAssociation(_Out_ HWND* window_handle) override;
  virtual HRESULT WINAPI
    CreateSwapChain(_In_ IUnknown* device,
                    _In_ DXGI_SWAP_CHAIN_DESC* desc,
                    _Out_ IDXGISwapChain** swap_chain) override;
  virtual HRESULT WINAPI
    CreateSoftwareAdapter(HMODULE module,
                          _Out_ IDXGIAdapter** adapter) override;

  // IDXGIFactory1
  virtual HRESULT WINAPI
    EnumAdapters1(UINT adapter_index, _Out_ IDXGIAdapter1** adapter) override;
  virtual BOOL WINAPI IsCurrent() override;

  // IDXGIFactory2
  virtual BOOL WINAPI IsWindowedStereoEnabled() override;
  virtual HRESULT WINAPI CreateSwapChainForHwnd(
    _In_ IUnknown* device,
    _In_ HWND hwnd,
    _In_ const DXGI_SWAP_CHAIN_DESC1* desc,
    _In_opt_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen_desc,
    _In_opt_ IDXGIOutput* restrict_to_output,
    _Out_ IDXGISwapChain1** swap_chain) override;
  virtual HRESULT WINAPI
    CreateSwapChainForCoreWindow(_In_ IUnknown* device,
                                 _In_ IUnknown* window,
                                 _In_ const DXGI_SWAP_CHAIN_DESC1* desc,
                                 _In_opt_ IDXGIOutput* restrict_to_output,
                                 _Out_ IDXGISwapChain1** swap_chain) override;
  virtual HRESULT WINAPI
    GetSharedResourceAdapterLuid(_In_ HANDLE resource,
                                 _Out_ LUID* luid) override;
  virtual HRESULT WINAPI
    RegisterStereoStatusWindow(_In_ HWND window_handle,
                               _In_ UINT msg,
                               _Out_ DWORD* cookie) override;
  virtual HRESULT WINAPI
    RegisterStereoStatusEvent(_In_ HANDLE event_handle,
                              _Out_ DWORD* cookie) override;
  virtual void WINAPI UnregisterStereoStatus(_In_ DWORD cookie) override;
  virtual HRESULT WINAPI
    RegisterOcclusionStatusWindow(_In_ HWND window_handle,
                                  _In_ UINT msg,
                                  _Out_ DWORD* cookie) override;
  virtual HRESULT WINAPI
    RegisterOcclusionStatusEvent(_In_ HANDLE event_handle,
                                 _Out_ DWORD* cookie) override;
  virtual void WINAPI UnregisterOcclusionStatus(_In_ DWORD cookie) override;
  virtual HRESULT WINAPI CreateSwapChainForComposition(
    _In_ IUnknown* device,
    _In_ const DXGI_SWAP_CHAIN_DESC1* desc,
    _In_opt_ IDXGIOutput* restrict_to_output,
    _Outptr_ IDXGISwapChain1** swap_chain) override;

protected:
  void Cleanup();

  std::int64_t refs_{1};
  IDXGIFactory* factory_{};
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)
}
}
