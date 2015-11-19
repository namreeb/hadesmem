// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "dxgi.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>

#include "callbacks.hpp"
#include "helpers.hpp"
#include "hook_counter.hpp"
#include "main.hpp"
#include "module.hpp"
#include "render_helper.hpp"

// TODO: Implement AddRef/Release support.

// TODO: Reimplement D3D10/D3D11 device OnRelease support.

// TODO: Hook IDXGISwapChain::ResizeBuffers. Also IDXGISwapChain::ResizeTarget?

namespace
{
class DXGIImpl : public hadesmem::cerberus::DXGIInterface
{
public:
  virtual std::size_t RegisterOnFrame(
    std::function<hadesmem::cerberus::OnFrameDXGICallback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnFrameDXGICallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnFrame(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnFrameDXGICallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnRelease(
    std::function<hadesmem::cerberus::OnReleaseDXGICallback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnReleaseDXGICallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnRelease(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnReleaseDXGICallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnResize(
    std::function<hadesmem::cerberus::OnResizeDXGICallback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnResizeDXGICallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnResize(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnResizeDXGICallbacks();
    return callbacks.Unregister(id);
  }
};

std::uint32_t& GetPresentHookCount() noexcept
{
  thread_local static std::uint32_t in_hook = 0;
  return in_hook;
}

typedef ULONG(WINAPI* IDXGISwapChain_Present_Fn)(IDXGISwapChain* swap_chain,
                                                 UINT sync_interval,
                                                 UINT flags);

std::unique_ptr<hadesmem::PatchDetour<IDXGISwapChain_Present_Fn>>&
  GetIDXGISwapChainPresentDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDXGISwapChain_Present_Fn>>
    detour;
  return detour;
}

typedef HRESULT(WINAPI* IDXGISwapChain_ResizeBuffers_Fn)(
  IDXGISwapChain* swap_chain,
  UINT buffer_count,
  UINT width,
  UINT height,
  DXGI_FORMAT new_format,
  UINT swap_chain_flags);

std::unique_ptr<hadesmem::PatchDetour<IDXGISwapChain_ResizeBuffers_Fn>>&
  GetIDXGISwapChainResizeBuffersDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDXGISwapChain_ResizeBuffers_Fn>>
    detour;
  return detour;
}

typedef ULONG(WINAPI* IDXGISwapChain1_Present1_Fn)(
  IDXGISwapChain1* swap_chain,
  UINT sync_interval,
  UINT present_flags,
  const DXGI_PRESENT_PARAMETERS* present_parameters);

std::unique_ptr<hadesmem::PatchDetour<IDXGISwapChain1_Present1_Fn>>&
  GetIDXGISwapChain1Present1Detour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDXGISwapChain1_Present1_Fn>>
    detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetDXGIModule() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

HRESULT WINAPI IDXGISwapChain_Present_Detour(hadesmem::PatchDetourBase* detour,
                                             IDXGISwapChain* swap_chain,
                                             UINT sync_interval,
                                             UINT flags)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;
  hadesmem::cerberus::HookCounter hook_counter{&GetPresentHookCount()};

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%u] [%u].", swap_chain, sync_interval, flags);

  if (hook_counter.GetCount() == 1)
  {
    auto& callbacks = hadesmem::cerberus::GetOnFrameDXGICallbacks();
    callbacks.Run(swap_chain);
  }

  auto const present = detour->GetTrampolineT<IDXGISwapChain_Present_Fn>();
  last_error_preserver.Revert();
  auto ret = present(swap_chain, sync_interval, flags);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

HRESULT WINAPI
  IDXGISwapChain_ResizeBuffers_Detour(hadesmem::PatchDetourBase* detour,
                                      IDXGISwapChain* swap_chain,
                                      UINT buffer_count,
                                      UINT width,
                                      UINT height,
                                      DXGI_FORMAT new_format,
                                      UINT swap_chain_flags)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%u] [%u] [%u] [%d] [%u].",
                                 swap_chain,
                                 buffer_count,
                                 width,
                                 height,
                                 new_format,
                                 swap_chain_flags);

  auto const resize_buffers =
    detour->GetTrampolineT<IDXGISwapChain_ResizeBuffers_Fn>();
  last_error_preserver.Revert();
  auto ret = resize_buffers(
    swap_chain, buffer_count, width, height, new_format, swap_chain_flags);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    auto& callbacks = hadesmem::cerberus::GetOnResizeDXGICallbacks();
    callbacks.Run(swap_chain, width, height);
  }

  return ret;
}

HRESULT WINAPI IDXGISwapChain1_Present1_Detour(
  hadesmem::PatchDetourBase* detour,
  IDXGISwapChain1* swap_chain,
  UINT sync_interval,
  UINT present_flags,
  const DXGI_PRESENT_PARAMETERS* present_parameters)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;
  hadesmem::cerberus::HookCounter hook_counter{&GetPresentHookCount()};

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%u] [%u] [%p].",
                                       this,
                                       sync_interval,
                                       present_flags,
                                       present_parameters);

  if (hook_counter.GetCount() == 1)
  {
    auto& callbacks = hadesmem::cerberus::GetOnFrameDXGICallbacks();
    callbacks.Run(swap_chain);
  }

  auto const present_1 = detour->GetTrampolineT<IDXGISwapChain1_Present1_Fn>();
  last_error_preserver.Revert();
  auto ret =
    present_1(swap_chain, sync_interval, present_flags, present_parameters);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}
}

namespace hadesmem
{
namespace cerberus
{
Callbacks<OnFrameDXGICallback>& GetOnFrameDXGICallbacks()
{
  static Callbacks<OnFrameDXGICallback> callbacks;
  return callbacks;
}

Callbacks<OnReleaseDXGICallback>& GetOnReleaseDXGICallbacks()
{
  static Callbacks<OnReleaseDXGICallback> callbacks;
  return callbacks;
}

Callbacks<OnResizeDXGICallback>& GetOnResizeDXGICallbacks()
{
  static Callbacks<OnResizeDXGICallback> callbacks;
  return callbacks;
}

DXGIInterface& GetDXGIInterface() noexcept
{
  static DXGIImpl dxgi_impl;
  return dxgi_impl;
}

void InitializeDXGI()
{
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(
    L"DXGI", &DetourDXGI, &UndetourDXGI, &GetDXGIModule);
}

void DetourDXGI(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetDXGIModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"DXGI", base, module))
  {
    auto& render_helper = GetRenderHelperInterface();
    auto const render_offsets = render_helper.GetRenderOffsets();
    auto const dxgi_offsets = &render_offsets->dxgi_offsets_;
    auto const offset_base = reinterpret_cast<std::uint8_t*>(base);

    if (!dxgi_offsets->present_)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "WARNING! No DXGI offsets. Skipping hooks.");
      return;
    }

    auto const present_fn = offset_base + dxgi_offsets->present_;
    DetourFunc(process,
               "IDXGISwapChain::Present",
               GetIDXGISwapChainPresentDetour(),
               reinterpret_cast<IDXGISwapChain_Present_Fn>(present_fn),
               IDXGISwapChain_Present_Detour);

    auto const resize_buffers_fn = offset_base + dxgi_offsets->resize_buffers_;
    DetourFunc(
      process,
      "IDXGISwapChain::ResizeBuffers",
      GetIDXGISwapChainResizeBuffersDetour(),
      reinterpret_cast<IDXGISwapChain_ResizeBuffers_Fn>(resize_buffers_fn),
      IDXGISwapChain_ResizeBuffers_Detour);

    auto const present_1_fn = offset_base + dxgi_offsets->present_1_;
    DetourFunc(process,
               "IDXGISwapChain1::Present1",
               GetIDXGISwapChain1Present1Detour(),
               reinterpret_cast<IDXGISwapChain1_Present1_Fn>(present_1_fn),
               IDXGISwapChain1_Present1_Detour);
  }
}

void UndetourDXGI(bool remove)
{
  auto& module = GetDXGIModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"DXGI", module))
  {
    UndetourFunc(
      L"IDXGISwapChain::Present", GetIDXGISwapChainPresentDetour(), remove);
    UndetourFunc(L"IDXGISwapChain::ResizeBuffers",
                 GetIDXGISwapChainResizeBuffersDetour(),
                 remove);
    UndetourFunc(
      L"IDXGISwapChain::Present1", GetIDXGISwapChain1Present1Detour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}
}
}
