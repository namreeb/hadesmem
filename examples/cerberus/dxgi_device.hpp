// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>

#include <windows.h>

#include <hadesmem/config.hpp>

#include <dxgi1_2.h>
#include <dxgi.h>

namespace hadesmem
{
namespace cerberus
{
class DXGIDeviceProxy : public IDXGIDevice2
{
public:
  explicit DXGIDeviceProxy(IDXGIDevice* device, void* d3d_device)
    : device_{device}, d3d_device_{d3d_device}
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

  // IDXGIDevice
  virtual HRESULT STDMETHODCALLTYPE
    GetAdapter(_Out_ IDXGIAdapter** pAdapter) override;
  virtual HRESULT STDMETHODCALLTYPE
    CreateSurface(_In_ const DXGI_SURFACE_DESC* pDesc,
                  UINT NumSurfaces,
                  DXGI_USAGE Usage,
                  _In_opt_ const DXGI_SHARED_RESOURCE* pSharedResource,
                  _Out_ IDXGISurface** ppSurface) override;
  virtual HRESULT STDMETHODCALLTYPE
    QueryResourceResidency(_In_reads_(NumResources)
                             IUnknown* const* ppResources,
                           _Out_writes_(NumResources)
                             DXGI_RESIDENCY* pResidencyStatus,
                           UINT NumResources) override;
  virtual HRESULT STDMETHODCALLTYPE SetGPUThreadPriority(INT Priority) override;
  virtual HRESULT STDMETHODCALLTYPE
    GetGPUThreadPriority(_Out_ INT* pPriority) override;

  // IDXGIDevice1
  virtual HRESULT STDMETHODCALLTYPE
    SetMaximumFrameLatency(UINT MaxLatency) /*override*/;
  virtual HRESULT STDMETHODCALLTYPE
    GetMaximumFrameLatency(_Out_ UINT* pMaxLatency) /*override*/;

  // IDXGIDevice2
  virtual HRESULT STDMETHODCALLTYPE
    OfferResources(_In_ UINT NumResources,
                   _In_reads_(NumResources) IDXGIResource* const* ppResources,
                   _In_ DXGI_OFFER_RESOURCE_PRIORITY Priority) /*override*/;
  virtual HRESULT STDMETHODCALLTYPE
    ReclaimResources(_In_ UINT NumResources,
                     _In_reads_(NumResources) IDXGIResource* const* ppResources,
                     _Out_writes_all_opt_(NumResources)
                       BOOL* pDiscarded) /*override*/;
  virtual HRESULT STDMETHODCALLTYPE
    EnqueueSetEvent(_In_ HANDLE hEvent) /*override*/;

protected:
  void Cleanup();

  std::int64_t refs_{1};
  IDXGIDevice* device_{};
  void* d3d_device_{};
};
}
}
