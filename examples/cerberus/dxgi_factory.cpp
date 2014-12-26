// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "dxgi_factory.hpp"

#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>

#include "dxgi.hpp"
#include "dxgi_swap_chain.hpp"

namespace hadesmem
{
namespace cerberus
{
HRESULT WINAPI DXGIFactoryProxy::QueryInterface(REFIID riid, void** obj)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  last_error_preserver.Revert();
  auto const ret = factory_->QueryInterface(riid, obj);
  last_error_preserver.Update();

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (*obj == factory_)
    {
      refs_++;
      *obj = this;
    }
    else
    {
#if !defined(HADESMEM_GCC)
      // Far Cry 4
      if (riid == __uuidof(IDXGIDisplayControl))
      {
        return ret;
      }
      else
#endif // #if !defined(HADESMEM_GCC)
      {
        HADESMEM_DETAIL_TRACE_A("WARNING! Unhandled interface.");
        HADESMEM_DETAIL_ASSERT(false);
        static_cast<IUnknown*>(*obj)->Release();
        return E_NOINTERFACE;
      }
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

ULONG WINAPI DXGIFactoryProxy::AddRef()
{
  refs_++;
  auto const ret = factory_->AddRef();
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);
  return ret;
}

ULONG WINAPI DXGIFactoryProxy::Release()
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  refs_--;
  HADESMEM_DETAIL_ASSERT(refs_ >= 0);

  if (refs_ == 0)
  {
    Cleanup();
  }

  last_error_preserver.Revert();
  auto const ret = factory_->Release();
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);

  if (ret == 0)
  {
    delete this;
  }

  return ret;
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::SetPrivateData(REFGUID name,
                                   UINT data_size,
                                   const void* data)
{
  return factory_->SetPrivateData(name, data_size, data);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::SetPrivateDataInterface(REFGUID name,
                                            const IUnknown* unknown)
{
  return factory_->SetPrivateDataInterface(name, unknown);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::GetPrivateData(REFGUID name, UINT* data_size, void* data)
{
  return factory_->GetPrivateData(name, data_size, data);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::GetParent(REFIID riid, void** parent)
{
  return factory_->GetParent(riid, parent);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::EnumAdapters(UINT adapter_index, IDXGIAdapter** adapter)
{
  return factory_->EnumAdapters(adapter_index, adapter);
}

HRESULT WINAPI
  DXGIFactoryProxy::MakeWindowAssociation(HWND window_handle, UINT flags)
{
  return factory_->MakeWindowAssociation(window_handle, flags);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::GetWindowAssociation(HWND* window_handle)
{
  return factory_->GetWindowAssociation(window_handle);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::CreateSwapChain(IUnknown* device,
                                    DXGI_SWAP_CHAIN_DESC* desc,
                                    IDXGISwapChain** swap_chain)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%p] [%p] [%p].", this, device, desc, swap_chain);

  last_error_preserver.Revert();
  auto const ret = factory_->CreateSwapChain(device, desc, swap_chain);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (swap_chain)
    {
      HADESMEM_DETAIL_TRACE_A("Proxying IDXGISwapChain.");
      *swap_chain = new DXGISwapChainProxy{*swap_chain};
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid swap chain out param pointer.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::CreateSoftwareAdapter(HMODULE module,
                                          IDXGIAdapter** adapter)
{
  return factory_->CreateSoftwareAdapter(module, adapter);
}

#if !defined(HADESMEM_GCC)
_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::EnumAdapters1(UINT adapter_index, IDXGIAdapter1** adapter)
{
  auto const factory = static_cast<IDXGIFactory1*>(factory_);
  return factory->EnumAdapters1(adapter_index, adapter);
}

BOOL WINAPI DXGIFactoryProxy::IsCurrent()
{
  auto const factory = static_cast<IDXGIFactory1*>(factory_);
  return factory->IsCurrent();
}

BOOL WINAPI DXGIFactoryProxy::IsWindowedStereoEnabled()
{
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  return factory->IsWindowedStereoEnabled();
}

_Use_decl_annotations_ HRESULT WINAPI DXGIFactoryProxy::CreateSwapChainForHwnd(
  IUnknown* device,
  HWND hwnd,
  const DXGI_SWAP_CHAIN_DESC1* desc,
  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen_desc,
  IDXGIOutput* restrict_to_output,
  IDXGISwapChain1** swap_chain)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p] [%p] [%p].",
                                 this,
                                 device,
                                 hwnd,
                                 desc,
                                 fullscreen_desc,
                                 restrict_to_output,
                                 swap_chain);

  last_error_preserver.Revert();
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  auto const ret = factory->CreateSwapChainForHwnd(
    device, hwnd, desc, fullscreen_desc, restrict_to_output, swap_chain);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (swap_chain)
    {
      HADESMEM_DETAIL_TRACE_A("Proxying IDXGISwapChain1.");
      *swap_chain = new DXGISwapChainProxy{*swap_chain};
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid swap chain out param pointer.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::CreateSwapChainForCoreWindow(
    IUnknown* device,
    IUnknown* window,
    const DXGI_SWAP_CHAIN_DESC1* desc,
    IDXGIOutput* restrict_to_output,
    IDXGISwapChain1** swap_chain)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p] [%p].",
                                 this,
                                 device,
                                 window,
                                 desc,
                                 restrict_to_output,
                                 swap_chain);

  last_error_preserver.Revert();
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  auto const ret = factory->CreateSwapChainForCoreWindow(
    device, window, desc, restrict_to_output, swap_chain);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (swap_chain)
    {
      HADESMEM_DETAIL_TRACE_A("Proxying IDXGISwapChain1.");
      *swap_chain = new DXGISwapChainProxy{*swap_chain};
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid swap chain out param pointer.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::GetSharedResourceAdapterLuid(HANDLE resource, LUID* luid)
{
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  return factory->GetSharedResourceAdapterLuid(resource, luid);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::RegisterStereoStatusWindow(HWND window_handle,
                                               UINT msg,
                                               DWORD* cookie)
{
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  return factory->RegisterStereoStatusWindow(window_handle, msg, cookie);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::RegisterStereoStatusEvent(HANDLE event_handle,
                                              DWORD* cookie)
{
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  return factory->RegisterStereoStatusEvent(event_handle, cookie);
}

_Use_decl_annotations_ void WINAPI
  DXGIFactoryProxy::UnregisterStereoStatus(DWORD cookie)
{
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  return factory->UnregisterStereoStatus(cookie);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::RegisterOcclusionStatusWindow(HWND window_handle,
                                                  UINT msg,
                                                  DWORD* cookie)
{
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  return factory->RegisterOcclusionStatusWindow(window_handle, msg, cookie);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::RegisterOcclusionStatusEvent(HANDLE event_handle,
                                                 DWORD* cookie)
{
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  return factory->RegisterOcclusionStatusEvent(event_handle, cookie);
}

_Use_decl_annotations_ void WINAPI
  DXGIFactoryProxy::UnregisterOcclusionStatus(DWORD cookie)
{
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  return factory->UnregisterOcclusionStatus(cookie);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGIFactoryProxy::CreateSwapChainForComposition(
    IUnknown* device,
    const DXGI_SWAP_CHAIN_DESC1* desc,
    IDXGIOutput* restrict_to_output,
    IDXGISwapChain1** swap_chain)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p].",
                                 this,
                                 device,
                                 desc,
                                 restrict_to_output,
                                 swap_chain);

  last_error_preserver.Revert();
  auto const factory = static_cast<IDXGIFactory2*>(factory_);
  auto const ret = factory->CreateSwapChainForComposition(
    device, desc, restrict_to_output, swap_chain);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (swap_chain)
    {
      HADESMEM_DETAIL_TRACE_A("Proxying IDXGISwapChain1.");
      *swap_chain = new DXGISwapChainProxy{*swap_chain};
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid swap chain out param pointer.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}
#endif // #if !defined(HADESMEM_GCC)

void DXGIFactoryProxy::Cleanup()
{
  HADESMEM_DETAIL_TRACE_A("Called.");
}
}
}
