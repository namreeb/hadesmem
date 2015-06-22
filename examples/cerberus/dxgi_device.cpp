// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "dxgi_device.hpp"

#include <d3d11_2.h>
#include <d3d11.h>

#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>

#include "d3d11_device.hpp"
#include "dxgi.hpp"
#include "dxgi_swap_chain.hpp"

namespace hadesmem
{
namespace cerberus
{
HRESULT WINAPI DXGIDeviceProxy::QueryInterface(REFIID riid, void** obj)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  last_error_preserver.Revert();
  auto const ret = device_->QueryInterface(riid, obj);
  last_error_preserver.Update();

  if (FAILED(ret))
  {
    HADESMEM_DETAIL_TRACE_NOISY_A("Failed.");
    return ret;
  }

  HADESMEM_DETAIL_TRACE_NOISY_A("Succeeded.");

  // DXGI internal GUID. Observed in ARK and probably others.
  UUID const unknown_uuid_1 = {
    0x9b7e4a00, 0x342c, 0x4106, 0xa1, 0x9f, 0x4f, 0x27, 0x04, 0xf6, 0x89, 0xf0};

  if (*obj == device_)
  {
    refs_++;
    *obj = this;
  }
  else if (riid == __uuidof(ID3D11Device) || riid == __uuidof(ID3D11Device1) ||
           riid == __uuidof(ID3D11Device2))
  {
    *obj = new D3D11DeviceProxy(static_cast<ID3D11Device*>(*obj));
  }
  else if (riid == unknown_uuid_1)
  {
    // Needs investigation to see if we need to wrap this.
    HADESMEM_DETAIL_TRACE_A("WARNING! Potentially unhandled interface (1).");
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("WARNING! Unhandled interface.");
    HADESMEM_DETAIL_ASSERT(false);
    static_cast<IUnknown*>(*obj)->Release();
    return E_NOINTERFACE;
  }

  return ret;
}

ULONG WINAPI DXGIDeviceProxy::AddRef()
{
  refs_++;
  auto const ret = device_->AddRef();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);
  return ret;
}

ULONG WINAPI DXGIDeviceProxy::Release()
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

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);

  if (ret == 0)
  {
    delete this;
  }

  return ret;
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::SetPrivateData(REFGUID name,
                                  UINT data_size,
                                  const void* data)
{
  return device_->SetPrivateData(name, data_size, data);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::SetPrivateDataInterface(REFGUID name,
                                           const IUnknown* unknown)
{
  return device_->SetPrivateDataInterface(name, unknown);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::GetPrivateData(REFGUID name, UINT* data_size, void* data)
{
  return device_->GetPrivateData(name, data_size, data);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::GetParent(REFIID riid, void** parent)
{
  return device_->GetParent(riid, parent);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::GetAdapter(IDXGIAdapter** pAdapter)
{
  return device_->GetAdapter(pAdapter);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::CreateSurface(const DXGI_SURFACE_DESC* pDesc,
                                 UINT NumSurfaces,
                                 DXGI_USAGE Usage,
                                 const DXGI_SHARED_RESOURCE* pSharedResource,
                                 IDXGISurface** ppSurface)
{
  return device_->CreateSurface(
    pDesc, NumSurfaces, Usage, pSharedResource, ppSurface);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::QueryResourceResidency(IUnknown* const* ppResources,
                                          DXGI_RESIDENCY* pResidencyStatus,
                                          UINT NumResources)
{
  return device_->QueryResourceResidency(
    ppResources, pResidencyStatus, NumResources);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::SetGPUThreadPriority(INT Priority)
{
  return device_->SetGPUThreadPriority(Priority);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::GetGPUThreadPriority(INT* pPriority)
{
  return device_->GetGPUThreadPriority(pPriority);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::SetMaximumFrameLatency(UINT MaxLatency)
{
  auto const device = static_cast<IDXGIDevice1*>(device_);
  return device->SetMaximumFrameLatency(MaxLatency);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::GetMaximumFrameLatency(UINT* pMaxLatency)
{
  auto const device = static_cast<IDXGIDevice1*>(device_);
  return device->GetMaximumFrameLatency(pMaxLatency);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::OfferResources(UINT NumResources,
                                  IDXGIResource* const* ppResources,
                                  DXGI_OFFER_RESOURCE_PRIORITY Priority)
{
  auto const device = static_cast<IDXGIDevice2*>(device_);
  return device->OfferResources(NumResources, ppResources, Priority);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::ReclaimResources(UINT NumResources,
                                    IDXGIResource* const* ppResources,
                                    BOOL* pDiscarded)
{
  auto const device = static_cast<IDXGIDevice2*>(device_);
  return device->ReclaimResources(NumResources, ppResources, pDiscarded);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIDeviceProxy::EnqueueSetEvent(HANDLE hEvent)
{
  auto const device = static_cast<IDXGIDevice2*>(device_);
  return device->EnqueueSetEvent(hEvent);
}

void DXGIDeviceProxy::Cleanup()
{
  HADESMEM_DETAIL_TRACE_A("Called.");
}
}
}
