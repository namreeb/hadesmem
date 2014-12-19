// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "dxgi_swap_chain.hpp"

#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>

#include "dxgi.hpp"

namespace hadesmem
{
namespace cerberus
{
HRESULT WINAPI DXGISwapChainProxy::QueryInterface(REFIID riid, void** obj)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  last_error_preserver.Revert();
  auto const ret = swap_chain_->QueryInterface(riid, obj);
  last_error_preserver.Update();

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (*obj == swap_chain_)
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

ULONG WINAPI DXGISwapChainProxy::AddRef()
{
  refs_++;
  auto const ret = swap_chain_->AddRef();
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Internal refs: [%lu]. External refs: [%lld].", ret, refs_);
  return ret;
}

ULONG WINAPI DXGISwapChainProxy::Release()
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  refs_--;
  HADESMEM_DETAIL_ASSERT(refs_ >= 0);

  if (refs_ == 0)
  {
    Cleanup();
  }

  last_error_preserver.Revert();
  auto const ret = swap_chain_->Release();
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
  DXGISwapChainProxy::SetPrivateData(REFGUID name,
                                     UINT data_size,
                                     const void* data)
{
  return swap_chain_->SetPrivateData(name, data_size, data);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::SetPrivateDataInterface(REFGUID name,
                                              const IUnknown* unknown)
{
  return swap_chain_->SetPrivateDataInterface(name, unknown);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetPrivateData(REFGUID name, UINT* data_size, void* data)
{
  return swap_chain_->GetPrivateData(name, data_size, data);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetParent(REFIID riid, void** parent)
{
  return swap_chain_->GetParent(riid, parent);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetDevice(REFIID riid, void** device)
{
  return swap_chain_->GetDevice(riid, device);
}

HRESULT WINAPI DXGISwapChainProxy::Present(UINT sync_interval, UINT flags)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%u] [%u].", this, sync_interval, flags);

  auto& callbacks = GetOnFrameDXGICallbacks();
  callbacks.Run(swap_chain_);

  last_error_preserver.Revert();
  auto const ret = swap_chain_->Present(sync_interval, flags);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetBuffer(UINT buffer, REFIID riid, void** surface)
{
  return swap_chain_->GetBuffer(buffer, riid, surface);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::SetFullscreenState(BOOL fullscreen, IDXGIOutput* target)
{
  return swap_chain_->SetFullscreenState(fullscreen, target);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetFullscreenState(BOOL* fullscreen, IDXGIOutput** target)
{
  return swap_chain_->GetFullscreenState(fullscreen, target);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetDesc(DXGI_SWAP_CHAIN_DESC* desc)
{
  return swap_chain_->GetDesc(desc);
}

HRESULT WINAPI DXGISwapChainProxy::ResizeBuffers(UINT buffer_count,
                                                 UINT width,
                                                 UINT height,
                                                 DXGI_FORMAT new_format,
                                                 UINT swap_chain_flags)
{
  return swap_chain_->ResizeBuffers(
    buffer_count, width, height, new_format, swap_chain_flags);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::ResizeTarget(const DXGI_MODE_DESC* new_target_parameters)
{
  return swap_chain_->ResizeTarget(new_target_parameters);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetContainingOutput(IDXGIOutput** output)
{
  return swap_chain_->GetContainingOutput(output);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetFrameStatistics(DXGI_FRAME_STATISTICS* stats)
{
  return swap_chain_->GetFrameStatistics(stats);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetLastPresentCount(UINT* last_present_count)
{
  return swap_chain_->GetLastPresentCount(last_present_count);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetDesc1(DXGI_SWAP_CHAIN_DESC1* desc)
{
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  return swap_chain->GetDesc1(desc);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* desc)
{
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  return swap_chain->GetFullscreenDesc(desc);
}

_Use_decl_annotations_ HRESULT WINAPI DXGISwapChainProxy::GetHwnd(HWND* hwnd)
{
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  return swap_chain->GetHwnd(hwnd);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetCoreWindow(REFIID refiid, void** unk)
{
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  return swap_chain->GetCoreWindow(refiid, unk);
}

_Use_decl_annotations_ HRESULT WINAPI DXGISwapChainProxy::Present1(
  UINT sync_interval,
  UINT present_flags,
  const DXGI_PRESENT_PARAMETERS* present_parameters)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%u] [%u] [%p].",
                                       this,
                                       sync_interval,
                                       present_flags,
                                       present_parameters);

  auto& callbacks = GetOnFrameDXGICallbacks();
  callbacks.Run(swap_chain_);

  last_error_preserver.Revert();
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  auto const ret =
    swap_chain->Present1(sync_interval, present_flags, present_parameters);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

BOOL WINAPI DXGISwapChainProxy::IsTemporaryMonoSupported()
{
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  return swap_chain->IsTemporaryMonoSupported();
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetRestrictToOutput(IDXGIOutput** restrict_to_output)
{
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  return swap_chain->GetRestrictToOutput(restrict_to_output);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::SetBackgroundColor(const DXGI_RGBA* color)
{
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  return swap_chain->SetBackgroundColor(color);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetBackgroundColor(DXGI_RGBA* color)
{
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  return swap_chain->GetBackgroundColor(color);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::SetRotation(DXGI_MODE_ROTATION rotation)
{
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  return swap_chain->SetRotation(rotation);
}

_Use_decl_annotations_ HRESULT WINAPI
  DXGISwapChainProxy::GetRotation(DXGI_MODE_ROTATION* rotation)
{
  auto const swap_chain = static_cast<IDXGISwapChain1*>(swap_chain_);
  return swap_chain->GetRotation(rotation);
}

void DXGISwapChainProxy::Cleanup()
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  auto& callbacks = GetOnReleaseDXGICallbacks();
  callbacks.Run(swap_chain_);
}
}
}
