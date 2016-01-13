// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "d3d9.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <d3d9.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_helpers.hpp>
#include <hadesmem/region.hpp>

#include "callbacks.hpp"
#include "helpers.hpp"
#include "hook_counter.hpp"
#include "main.hpp"
#include "module.hpp"
#include "render_helper.hpp"

// TODO: Clean up code duplication caused by adding device to the map in all
// funcs. (Use a helper func instead.)

// TODO: Implement AddRef/Release support.

// TODO: Fix up the ref counting, reentrancy issues, etc. Everything is a major
// hack right now...

// TODO: Fix various race conditions in our hooks.

// TODO: Add resize detection/support for D3D9.

// TODO: Ensure we set up the D3D9 SDK correctly. http://bit.ly/1KaVNwO

// TODO: Add a mechanism to select EndScene OR Present OR PresentEx so we're not
// double/triple processing frames! Perhaps as soon as we get a frame one one of
// them, we 'disable' the rest. There should also be an override mechanism in
// the config as well as one to reset the state at runtime (for testing
// purposes).

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameD3D9Callback>&
  GetOnFrameD3D9Callbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameD3D9Callback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnResetD3D9Callback>&
  GetOnResetD3D9Callbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnResetD3D9Callback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnReleaseD3D9Callback>&
  GetOnReleaseD3D9Callbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnReleaseD3D9Callback> callbacks;
  return callbacks;
}

class D3D9Impl : public hadesmem::cerberus::D3D9Interface
{
public:
  virtual std::size_t RegisterOnFrame(
    std::function<hadesmem::cerberus::OnFrameD3D9Callback> const& callback)
    final
  {
    auto& callbacks = GetOnFrameD3D9Callbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnFrame(std::size_t id) final
  {
    auto& callbacks = GetOnFrameD3D9Callbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnReset(
    std::function<hadesmem::cerberus::OnResetD3D9Callback> const& callback)
    final
  {
    auto& callbacks = GetOnResetD3D9Callbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnReset(std::size_t id) final
  {
    auto& callbacks = GetOnResetD3D9Callbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnRelease(
    std::function<hadesmem::cerberus::OnReleaseD3D9Callback> const& callback)
    final
  {
    auto& callbacks = GetOnReleaseD3D9Callbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnRelease(std::size_t id) final
  {
    auto& callbacks = GetOnReleaseD3D9Callbacks();
    return callbacks.Unregister(id);
  }
};

std::uint32_t& GetPresentHookCount() noexcept
{
  thread_local static std::uint32_t in_hook = 0;
  return in_hook;
}

std::uint32_t& GetResetHookCount() noexcept
{
  thread_local static std::uint32_t in_hook = 0;
  return in_hook;
}

struct DeviceData
{
  std::int64_t ref_count_;
};

std::map<IDirect3DDevice9*, DeviceData>& GetDeviceMap()
{
  static std::map<IDirect3DDevice9*, DeviceData> device_map;
  return device_map;
}

std::recursive_mutex& GetDeviceMapMutex()
{
  static std::recursive_mutex mutex;
  return mutex;
}

void AddDeviceToMap(IDirect3DDevice9* device)
{
  auto& mutex = GetDeviceMapMutex();
  std::lock_guard<std::recursive_mutex> lock{mutex};

  auto& map = GetDeviceMap();
  auto const iter = map.find(device);
  if (iter == std::end(map))
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("Adding new device. Device: [%p].", device);

    DeviceData data = {};
    data.ref_count_ = 1;
    map[device] = data;
  }
}

std::pair<bool, DeviceData> GetDeviceDataCopy(IDirect3DDevice9* device)
{
  auto& mutex = GetDeviceMapMutex();
  std::lock_guard<std::recursive_mutex> lock{mutex};

  auto& map = GetDeviceMap();
  auto const iter = map.find(device);
  if (iter != std::end(map))
  {
    return {true, iter->second};
  }

  return {};
}

void EraseDeviceData(IDirect3DDevice9* device)
{
  auto& mutex = GetDeviceMapMutex();
  std::lock_guard<std::recursive_mutex> lock{mutex};

  auto& map = GetDeviceMap();
  auto const iter = map.find(device);
  if (iter != std::end(map))
  {
    map.erase(iter);
  }
}

typedef ULONG(WINAPI* IDirect3DDevice9_AddRef_Fn)(IDirect3DDevice9* device);

std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9_AddRef_Fn>>&
  GetIDirect3DDevice9AddRefDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9_AddRef_Fn>>
    detour;
  return detour;
}

extern "C" ULONG WINAPI IDirect3DDevice9_AddRef_Detour(
  hadesmem::PatchDetourBase* detour, IDirect3DDevice9* device)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  if (GetPresentHookCount() == 0)
  {
    auto& mutex = GetDeviceMapMutex();
    std::lock_guard<std::recursive_mutex> lock{mutex};

    auto& map = GetDeviceMap();
    auto const iter = map.find(device);
    if (iter != std::end(map))
    {
      auto const ref_count = ++iter->second.ref_count_;
      (void)ref_count;
      HADESMEM_DETAIL_ASSERT(ref_count > 0);
    }
  }

  auto const add_ref = detour->GetTrampolineT<IDirect3DDevice9_AddRef_Fn>();
  last_error_preserver.Revert();
  auto ret = add_ref(device);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

typedef ULONG(WINAPI* IDirect3DDevice9_Release_Fn)(IDirect3DDevice9* device);

std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9_Release_Fn>>&
  GetIDirect3DDevice9ReleaseDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9_Release_Fn>>
    detour;
  return detour;
}

extern "C" ULONG WINAPI IDirect3DDevice9_Release_Detour(
  hadesmem::PatchDetourBase* detour, IDirect3DDevice9* device)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  auto device_data = GetDeviceDataCopy(device);
  if (device_data.first && GetPresentHookCount() == 0)
  {
    if (device_data.second.ref_count_ == 1)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A("Detected device release. Device: [%p].",
                                     device);

      EraseDeviceData(device);

      // TODO: Fix ref count mismatch causing this to be spammed on certain
      // games.
      // auto& callbacks = hadesmem::cerberus::GetOnReleaseD3D9Callbacks();
      // callbacks.Run(device);

      HADESMEM_DETAIL_TRACE_A("Finished running OnRelease callbacks.");
    }
    else
    {
      auto& mutex = GetDeviceMapMutex();
      std::lock_guard<std::recursive_mutex> lock{mutex};

      auto& map = GetDeviceMap();
      auto const iter = map.find(device);
      if (iter != std::end(map))
      {
        // TODO: Race condition here if being AddRef'd from a different thread?
        auto const ref_count = --iter->second.ref_count_;
        (void)ref_count;
        HADESMEM_DETAIL_ASSERT(ref_count > 0);
      }
    }
  }

  auto const release = detour->GetTrampolineT<IDirect3DDevice9_Release_Fn>();
  last_error_preserver.Revert();
  auto ret = release(device);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

typedef HRESULT(WINAPI* IDirect3DDevice9_EndScene_Fn)(IDirect3DDevice9* device);

std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9_EndScene_Fn>>&
  GetIDirect3DDevice9EndSceneDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9_EndScene_Fn>>
    detour;
  return detour;
}

extern "C" HRESULT WINAPI IDirect3DDevice9_EndScene_Detour(
  hadesmem::PatchDetourBase* detour, IDirect3DDevice9* device)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;
  hadesmem::cerberus::HookCounter hook_counter{&GetPresentHookCount()};

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  auto const hook_count = hook_counter.GetCount();
  HADESMEM_DETAIL_ASSERT(hook_count > 0);
  if (hook_count == 1)
  {
    AddDeviceToMap(device);

    auto& callbacks = GetOnFrameD3D9Callbacks();
    callbacks.Run(device);
  }

  auto const end_scene = detour->GetTrampolineT<IDirect3DDevice9_EndScene_Fn>();
  last_error_preserver.Revert();
  auto ret = end_scene(device);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

typedef HRESULT(WINAPI* IDirect3DDevice9_Present_Fn)(
  IDirect3DDevice9* device,
  const RECT* pSourceRect,
  const RECT* pDestRect,
  HWND hDestWindowOverride,
  const RGNDATA* pDirtyRegion);

std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9_Present_Fn>>&
  GetIDirect3DDevice9PresentDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9_Present_Fn>>
    detour;
  return detour;
}

extern "C" HRESULT WINAPI
  IDirect3DDevice9_Present_Detour(hadesmem::PatchDetourBase* detour,
                                  IDirect3DDevice9* device,
                                  const RECT* source_rect,
                                  const RECT* dest_rect,
                                  HWND dest_window_override,
                                  const RGNDATA* dirty_region)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;
  hadesmem::cerberus::HookCounter hook_counter{&GetPresentHookCount()};

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p].",
                                       device,
                                       source_rect,
                                       dest_rect,
                                       dest_window_override,
                                       dirty_region);

  auto const hook_count = hook_counter.GetCount();
  HADESMEM_DETAIL_ASSERT(hook_count > 0);
  if (hook_count == 1)
  {
    AddDeviceToMap(device);

// Disabled until we stop processing the same frame multiple times. (See TODO at
// top of file.)
#if 0
    auto& callbacks = GetOnFrameD3D9Callbacks();
    callbacks.Run(device);
#endif
  }

  auto const present = detour->GetTrampolineT<IDirect3DDevice9_Present_Fn>();
  last_error_preserver.Revert();
  auto ret =
    present(device, source_rect, dest_rect, dest_window_override, dirty_region);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

typedef HRESULT(WINAPI* IDirect3DDevice9Ex_PresentEx_Fn)(
  IDirect3DDevice9Ex* device,
  const RECT* pSourceRect,
  const RECT* pDestRect,
  HWND hDestWindowOverride,
  const RGNDATA* pDirtyRegion,
  DWORD dwFlags);

std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9Ex_PresentEx_Fn>>&
  GetIDirect3DDevice9ExPresentExDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9Ex_PresentEx_Fn>>
    detour;
  return detour;
}

extern "C" HRESULT WINAPI
  IDirect3DDevice9Ex_PresentEx_Detour(hadesmem::PatchDetourBase* detour,
                                      IDirect3DDevice9Ex* device,
                                      const RECT* source_rect,
                                      const RECT* dest_rect,
                                      HWND dest_window_override,
                                      const RGNDATA* dirty_region,
                                      DWORD flags)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;
  hadesmem::cerberus::HookCounter hook_counter{&GetPresentHookCount()};

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p] [%lu].",
                                       device,
                                       source_rect,
                                       dest_rect,
                                       dest_window_override,
                                       dirty_region,
                                       flags);

  auto const hook_count = hook_counter.GetCount();
  HADESMEM_DETAIL_ASSERT(hook_count > 0);
  if (hook_count == 1)
  {
    AddDeviceToMap(device);

// Disabled until we stop processing the same frame multiple times. (See TODO at
// top of file.)
#if 0
    auto& callbacks = GetOnFrameD3D9Callbacks();
    callbacks.Run(device);
#endif
  }

  auto const present =
    detour->GetTrampolineT<IDirect3DDevice9Ex_PresentEx_Fn>();
  last_error_preserver.Revert();
  auto ret = present(
    device, source_rect, dest_rect, dest_window_override, dirty_region, flags);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

typedef HRESULT(WINAPI* IDirect3DSwapChain9_Present_Fn)(
  IDirect3DSwapChain9* swap_chain,
  const RECT* pSourceRect,
  const RECT* pDestRect,
  HWND hDestWindowOverride,
  const RGNDATA* pDirtyRegion,
  DWORD dwFlags);

std::unique_ptr<hadesmem::PatchDetour<IDirect3DSwapChain9_Present_Fn>>&
  GetIDirect3DSwapChain9PresentDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDirect3DSwapChain9_Present_Fn>>
    detour;
  return detour;
}

extern "C" HRESULT WINAPI
  IDirect3DSwapChain9_Present_Detour(hadesmem::PatchDetourBase* detour,
                                     IDirect3DSwapChain9* swap_chain,
                                     const RECT* source_rect,
                                     const RECT* dest_rect,
                                     HWND dest_window_override,
                                     const RGNDATA* dirty_region,
                                     DWORD flags)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;
  hadesmem::cerberus::HookCounter hook_counter{&GetPresentHookCount()};

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%p] [%p] [%p] [%p] [%lu].",
                                       swap_chain,
                                       source_rect,
                                       dest_rect,
                                       dest_window_override,
                                       dirty_region,
                                       flags);

  IDirect3DDevice9* device = nullptr;
  auto const get_device_hr = swap_chain->GetDevice(&device);
  if (FAILED(get_device_hr))
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "WARNING! IDirect3DSwapChain9::GetDevice failed. HR: [%lX].",
      get_device_hr);
  }
  hadesmem::detail::SmartComHandle smart_device{device};

  auto const hook_count = hook_counter.GetCount();
  HADESMEM_DETAIL_ASSERT(hook_count > 0);
  if (hook_count == 1 && SUCCEEDED(get_device_hr))
  {
    AddDeviceToMap(device);

// Disabled until we stop processing the same frame multiple times. (See TODO at
// top of file.)
#if 0
    auto& callbacks = GetOnFrameD3D9Callbacks();
    callbacks.Run(device);
#endif
  }

  auto const present = detour->GetTrampolineT<IDirect3DSwapChain9_Present_Fn>();
  last_error_preserver.Revert();
  auto ret = present(swap_chain,
                     source_rect,
                     dest_rect,
                     dest_window_override,
                     dirty_region,
                     flags);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

typedef HRESULT(WINAPI* IDirect3DDevice9_Reset_Fn)(
  IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters);

std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9_Reset_Fn>>&
  GetIDirect3DDevice9ResetDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9_Reset_Fn>>
    detour;
  return detour;
}

extern "C" HRESULT WINAPI
  IDirect3DDevice9_Reset_Detour(hadesmem::PatchDetourBase* detour,
                                IDirect3DDevice9* device,
                                D3DPRESENT_PARAMETERS* presentation_params)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;
  hadesmem::cerberus::HookCounter hook_counter{&GetResetHookCount()};

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%p].", device, presentation_params);

  auto const hook_count = hook_counter.GetCount();
  HADESMEM_DETAIL_ASSERT(hook_count > 0);
  if (hook_count == 1)
  {
    auto& callbacks = GetOnResetD3D9Callbacks();
    callbacks.Run(device, presentation_params);
  }

  auto const reset = detour->GetTrampolineT<IDirect3DDevice9_Reset_Fn>();
  last_error_preserver.Revert();
  auto ret = reset(device, presentation_params);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

typedef HRESULT(WINAPI* IDirect3DDevice9Ex_ResetEx_Fn)(
  IDirect3DDevice9Ex* device,
  D3DPRESENT_PARAMETERS* pPresentationParameters,
  D3DDISPLAYMODEEX* pFullscreenDisplayMode);

std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9Ex_ResetEx_Fn>>&
  GetIDirect3DDevice9ExResetExDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<IDirect3DDevice9Ex_ResetEx_Fn>>
    detour;
  return detour;
}

extern "C" HRESULT WINAPI
  IDirect3DDevice9Ex_ResetEx_Detour(hadesmem::PatchDetourBase* detour,
                                    IDirect3DDevice9Ex* device,
                                    D3DPRESENT_PARAMETERS* presentation_params,
                                    D3DDISPLAYMODEEX* fullscreen_display_mode)
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;
  hadesmem::cerberus::HookCounter hook_counter{&GetResetHookCount()};

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%p] [%p].",
                                       device,
                                       presentation_params,
                                       fullscreen_display_mode);

  auto const hook_count = hook_counter.GetCount();
  HADESMEM_DETAIL_ASSERT(hook_count > 0);
  if (hook_count == 1)
  {
    auto& callbacks = GetOnResetD3D9Callbacks();
    callbacks.Run(device, presentation_params);
  }

  auto const reset_ex = detour->GetTrampolineT<IDirect3DDevice9Ex_ResetEx_Fn>();
  last_error_preserver.Revert();
  auto ret = reset_ex(device, presentation_params, fullscreen_display_mode);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

std::pair<void*, SIZE_T>& GetD3D9Module() noexcept
{
  static std::pair<void*, SIZE_T> module{};
  return module;
}
}

namespace hadesmem
{
namespace cerberus
{
D3D9Interface& GetD3D9Interface() noexcept
{
  static D3D9Impl d3d9_impl;
  return d3d9_impl;
}

void InitializeD3D9()
{
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(
    L"D3D9", DetourD3D9, UndetourD3D9, GetD3D9Module);
}

void DetourD3D9(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetD3D9Module();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"D3D9", base, module))
  {
    auto& render_helper = GetRenderHelperInterface();
    auto const render_offsets = render_helper.GetRenderOffsets();
    auto const d3d9_offsets = &render_offsets->d3d9_offsets_;
    auto const offset_base = reinterpret_cast<std::uint8_t*>(base);

    if (!d3d9_offsets->add_ref_)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "WARNING! No D3D9 offsets. Skipping hooks.");
      return;
    }

    auto const add_ref_fn = offset_base + d3d9_offsets->add_ref_;
    DetourFunc(process,
               "IDirect3DDevice9::AddRef",
               GetIDirect3DDevice9AddRefDetour(),
               reinterpret_cast<IDirect3DDevice9_AddRef_Fn>(add_ref_fn),
               IDirect3DDevice9_AddRef_Detour);

    auto const release_fn = offset_base + d3d9_offsets->release_;
    DetourFunc(process,
               "IDirect3DDevice9::Release",
               GetIDirect3DDevice9ReleaseDetour(),
               reinterpret_cast<IDirect3DDevice9_Release_Fn>(release_fn),
               IDirect3DDevice9_Release_Detour);

    auto const present_fn = offset_base + d3d9_offsets->present_;
    DetourFunc(process,
               "IDirect3DDevice9::Present",
               GetIDirect3DDevice9PresentDetour(),
               reinterpret_cast<IDirect3DDevice9_Present_Fn>(present_fn),
               IDirect3DDevice9_Present_Detour);

    auto const reset_fn = offset_base + d3d9_offsets->reset_;
    DetourFunc(process,
               "IDirect3DDevice9::Reset",
               GetIDirect3DDevice9ResetDetour(),
               reinterpret_cast<IDirect3DDevice9_Reset_Fn>(reset_fn),
               IDirect3DDevice9_Reset_Detour);

    auto const end_scene_fn = offset_base + d3d9_offsets->end_scene_;
    DetourFunc(process,
               "IDirect3DDevice9::EndScene",
               GetIDirect3DDevice9EndSceneDetour(),
               reinterpret_cast<IDirect3DDevice9_EndScene_Fn>(end_scene_fn),
               IDirect3DDevice9_EndScene_Detour);

    auto const present_ex_fn = offset_base + d3d9_offsets->present_ex_;
    DetourFunc(process,
               "IDirect3DDevice9Ex::PresentEx",
               GetIDirect3DDevice9ExPresentExDetour(),
               reinterpret_cast<IDirect3DDevice9Ex_PresentEx_Fn>(present_ex_fn),
               IDirect3DDevice9Ex_PresentEx_Detour);

    auto const reset_ex_fn = offset_base + d3d9_offsets->reset_ex_;
    DetourFunc(process,
               "IDirect3DDevice9Ex::ResetEx",
               GetIDirect3DDevice9ExResetExDetour(),
               reinterpret_cast<IDirect3DDevice9Ex_ResetEx_Fn>(reset_ex_fn),
               IDirect3DDevice9Ex_ResetEx_Detour);

    auto const swap_chain_present_fn =
      offset_base + d3d9_offsets->swap_chain_present_;
    DetourFunc(
      process,
      "IDirect3DSwapChain9::Present",
      GetIDirect3DSwapChain9PresentDetour(),
      reinterpret_cast<IDirect3DSwapChain9_Present_Fn>(swap_chain_present_fn),
      IDirect3DSwapChain9_Present_Detour);
  }
}

void UndetourD3D9(bool remove)
{
  auto& module = GetD3D9Module();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"D3D9", module))
  {
    UndetourFunc(
      L"IDirect3DDevice9::AddRef", GetIDirect3DDevice9AddRefDetour(), remove);
    UndetourFunc(
      L"IDirect3DDevice9::Release", GetIDirect3DDevice9ReleaseDetour(), remove);
    UndetourFunc(
      L"IDirect3DDevice9::Present", GetIDirect3DDevice9PresentDetour(), remove);
    UndetourFunc(
      L"IDirect3DDevice9::Reset", GetIDirect3DDevice9ResetDetour(), remove);
    UndetourFunc(L"IDirect3DDevice9::EndScene",
                 GetIDirect3DDevice9EndSceneDetour(),
                 remove);
    UndetourFunc(L"IDirect3DDevice9Ex::PresentEx",
                 GetIDirect3DDevice9ExPresentExDetour(),
                 remove);
    UndetourFunc(L"IDirect3DDevice9Ex::ResetEx",
                 GetIDirect3DDevice9ExResetExDetour(),
                 remove);
    UndetourFunc(L"IDirect3DSwapChain9::Present",
                 GetIDirect3DSwapChain9PresentDetour(),
                 remove);

    module = std::make_pair(nullptr, 0);
  }
}
}
}
