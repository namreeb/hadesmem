// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>

#include <windows.h>

#include <hadesmem/config.hpp>

#if !defined(HADESMEM_GCC)
#include <dxgi1_2.h>
#endif // #if !defined(HADESMEM_GCC)
#include <dxgi.h>

#include "no_sal.hpp"

namespace hadesmem
{
namespace cerberus
{
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif // #if defined(HADESMEM_GCC)

#if defined(HADESMEM_GCC)
class DXGISwapChainProxy : public IDXGISwapChain
#else  // #if defined(HADESMEM_GCC)
class DXGISwapChainProxy : public IDXGISwapChain1
#endif // #if defined(HADESMEM_GCC)
{
public:
  explicit DXGISwapChainProxy(IDXGISwapChain* swap_chain)
    : swap_chain_{swap_chain}
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

  // IDXGIDeviceSubObject
  virtual HRESULT WINAPI
    GetDevice(_In_ REFIID riid, _Out_ void** device) override;

  // IDXGISwapChain
  virtual HRESULT WINAPI Present(UINT sync_interval, UINT flags) override;
  virtual HRESULT WINAPI
    GetBuffer(UINT buffer, _In_ REFIID riid, _Out_ void**) override;
  virtual HRESULT WINAPI
    SetFullscreenState(BOOL fullscreen, _In_opt_ IDXGIOutput* target) override;
  virtual HRESULT WINAPI
    GetFullscreenState(_Out_opt_ BOOL* fullscreen,
                       _Out_opt_ IDXGIOutput** target) override;
  virtual HRESULT WINAPI GetDesc(_Out_ DXGI_SWAP_CHAIN_DESC* desc) override;
  virtual HRESULT WINAPI ResizeBuffers(UINT buffer_count,
                                       UINT width,
                                       UINT height,
                                       DXGI_FORMAT new_format,
                                       UINT swap_chain_flags) override;
  virtual HRESULT WINAPI
    ResizeTarget(_In_ const DXGI_MODE_DESC* new_target_parameters) override;
  virtual HRESULT WINAPI
    GetContainingOutput(_Out_ IDXGIOutput** output) override;
  virtual HRESULT WINAPI
    GetFrameStatistics(_Out_ DXGI_FRAME_STATISTICS* stats) override;
  virtual HRESULT WINAPI
    GetLastPresentCount(_Out_ UINT* last_present_count) override;

#if !defined(HADESMEM_GCC)
  // IDXGISwapChain1
  virtual HRESULT WINAPI
    GetDesc1(_Out_ DXGI_SWAP_CHAIN_DESC1* desc) override;
  virtual HRESULT WINAPI
    GetFullscreenDesc(_Out_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC* desc) override;
  virtual HRESULT WINAPI GetHwnd(_Out_ HWND* hwnd) override;
  virtual HRESULT WINAPI
    GetCoreWindow(_In_ REFIID refiid, _Out_ void** unk) override;
  virtual HRESULT WINAPI Present1(
    UINT sync_interval,
    UINT present_flags,
    _In_ const DXGI_PRESENT_PARAMETERS* present_parameters) override;
  virtual BOOL WINAPI IsTemporaryMonoSupported() override;
  virtual HRESULT WINAPI
    GetRestrictToOutput(_Out_ IDXGIOutput** restrict_to_output) override;
  virtual HRESULT WINAPI
    SetBackgroundColor(_In_ const DXGI_RGBA* color) override;
  virtual HRESULT WINAPI
    GetBackgroundColor(_Out_ DXGI_RGBA* color) override;
  virtual HRESULT WINAPI
    SetRotation(_In_ DXGI_MODE_ROTATION rotation) override;
  virtual HRESULT WINAPI
    GetRotation(_Out_ DXGI_MODE_ROTATION* rotation) override;
#endif // #if !defined(HADESMEM_GCC)

protected:
  void Cleanup();

  std::int64_t refs_{1};
  IDXGISwapChain* swap_chain_{};
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)
}
}
