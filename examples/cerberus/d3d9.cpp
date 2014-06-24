// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d9.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <d3d9.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/detour_ref_counter.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>

#include "callbacks.hpp"
#include "helpers.hpp"
#include "main.hpp"
#include "module.hpp"

namespace
{

std::unique_ptr<hadesmem::PatchDetour>& GetDirect3DCreate9Detour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetDirect3DCreate9RefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetIDirect3D9CreateDeviceDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetIDirect3D9CreateDeviceRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetIDirect3DDevice9EndSceneDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetIDirect3DDevice9EndSceneRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::pair<void*, SIZE_T>& GetD3D9Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackD3D9>&
  GetOnFrameCallbacksD3D9()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackD3D9>
    callbacks;
  return callbacks;
}

extern "C" HRESULT WINAPI
  IDirect3DDevice9EndSceneDetour(IDirect3DDevice9* device)
  HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{
    GetIDirect3DDevice9EndSceneRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  try
  {
    auto& callbacks = GetOnFrameCallbacksD3D9();
    callbacks.Run(device);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  auto& detour = GetIDirect3DDevice9EndSceneDetour();
  auto const end_scene =
    detour->GetTrampoline<decltype(&IDirect3DDevice9EndSceneDetour)>();
  last_error_preserver.Revert();
  auto const ret = end_scene(device);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  return ret;
}

void DetourDirect3DDevice9(IDirect3DDevice9* device)
{
  try
  {
    auto& end_scene_detour = GetIDirect3DDevice9EndSceneDetour();
    if (!end_scene_detour)
    {
      void** const device_vtable = *reinterpret_cast<void***>(device);
      auto const end_scene = device_vtable[42];
      auto const end_scene_detour_fn =
        hadesmem::detail::UnionCast<void*>(&IDirect3DDevice9EndSceneDetour);
      end_scene_detour = std::make_unique<hadesmem::PatchDetour>(
        hadesmem::cerberus::GetThisProcess(), end_scene, end_scene_detour_fn);
      end_scene_detour->Apply();
      HADESMEM_DETAIL_TRACE_A("IDirect3DDevice9::EndScene detoured.");
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("IDirect3DDevice9::EndScene already detoured.");
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

extern "C" HRESULT WINAPI
  IDirect3D9CreateDeviceDetour(IDirect3D9* direct3d9,
                               UINT adapter,
                               D3DDEVTYPE device_type,
                               HWND focus_window,
                               DWORD behavior_flags,
                               D3DPRESENT_PARAMETERS* presentation_params,
                               IDirect3DDevice9** returned_device)
  HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{
    GetIDirect3D9CreateDeviceRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%u] [%d] [%p] [%u] [%p] [%p].",
                                 direct3d9,
                                 adapter,
                                 device_type,
                                 focus_window,
                                 behavior_flags,
                                 presentation_params,
                                 returned_device);
  auto& detour = GetIDirect3D9CreateDeviceDetour();
  auto const create_swap_chain =
    detour->GetTrampoline<decltype(&IDirect3D9CreateDeviceDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_swap_chain(direct3d9,
                                     adapter,
                                     device_type,
                                     focus_window,
                                     behavior_flags,
                                     presentation_params,
                                     returned_device);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    if (returned_device)
    {
      DetourDirect3DDevice9(*returned_device);
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Invalid device out param pointer.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}

void DetourDirect3D9(IDirect3D9* direct3d9)
{
  try
  {
    auto& create_device_detour = GetIDirect3D9CreateDeviceDetour();
    if (!create_device_detour)
    {
      void** const direct3d9_vtable = *reinterpret_cast<void***>(direct3d9);
      auto const create_device = direct3d9_vtable[16];
      auto const create_device_detour_fn =
        hadesmem::detail::UnionCast<void*>(&IDirect3D9CreateDeviceDetour);
      create_device_detour = std::make_unique<hadesmem::PatchDetour>(
        hadesmem::cerberus::GetThisProcess(),
        create_device,
        create_device_detour_fn);
      create_device_detour->Apply();
      HADESMEM_DETAIL_TRACE_A("IDirect3D9::CreateDevice detoured.");
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("IDirect3D9::CreateDevice already detoured.");
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

extern "C" IDirect3D9* WINAPI Direct3DCreate9Detour(UINT sdk_version)
  HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::DetourRefCounter ref_count{GetDirect3DCreate9RefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%u].", sdk_version);
  auto& detour = GetDirect3DCreate9Detour();
  auto const direct3d_create_9 =
    detour->GetTrampoline<decltype(&Direct3DCreate9Detour)>();
  last_error_preserver.Revert();
  auto const ret = direct3d_create_9(sdk_version);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%p].", ret);

  if (ret)
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    DetourDirect3D9(ret);
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Failed.");
  }

  return ret;
}
}

namespace hadesmem
{

namespace cerberus
{

void InitializeD3D9()
{
  InitializeSupportForModule(
    L"D3D9", &DetourD3D9, &UndetourD3D9, &GetD3D9Module);
}

void DetourD3D9(HMODULE base)
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  auto& module = GetD3D9Module();
  if (module.first)
  {
    HADESMEM_DETAIL_TRACE_A("D3D9 already detoured.");
    return;
  }

  if (!base)
  {
    base = ::GetModuleHandleW(L"d3d9");
  }

  if (!base)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to find D3D9 module.");
    return;
  }

  auto const& process = GetThisProcess();

  module =
    std::make_pair(base, hadesmem::detail::GetRegionAllocSize(process, base));

  if (!GetDirect3DCreate9Detour())
  {
    auto const orig_fn =
      detail::GetProcAddressInternal(process, base, "Direct3DCreate9");
    if (orig_fn)
    {
      auto const detour_fn = detail::UnionCast<void*>(&Direct3DCreate9Detour);
      auto& detour = GetDirect3DCreate9Detour();
      detour.reset(new PatchDetour(process, orig_fn, detour_fn));
      detour->Apply();
      HADESMEM_DETAIL_TRACE_A("Direct3DCreate9 detoured.");
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Could not find Direct3DCreate9 export.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Direct3DCreate9 already detoured.");
  }
}

void UndetourD3D9(bool remove)
{
  auto& module = GetD3D9Module();
  if (!module.first)
  {
    HADESMEM_DETAIL_TRACE_A("D3D9 not detoured.");
    return;
  }

  UndetourFunc(L"Direct3DCreate9",
               GetDirect3DCreate9Detour(),
               GetDirect3DCreate9RefCount(),
               remove);
  UndetourFunc(L"IDirect3D9::CreateDevice",
               GetIDirect3D9CreateDeviceDetour(),
               GetIDirect3D9CreateDeviceRefCount(),
               remove);
  UndetourFunc(L"IDirect3DDevice9::EndScene",
               GetIDirect3DDevice9EndSceneDetour(),
               GetIDirect3DDevice9EndSceneRefCount(),
               remove);

  module = std::make_pair(nullptr, 0);
}

std::size_t RegisterOnFrameCallbackD3D9(
  std::function<OnFrameCallbackD3D9> const& callback)
{
  auto& callbacks = GetOnFrameCallbacksD3D9();
  return callbacks.Register(callback);
}

void UnregisterOnFrameCallbackD3D9(std::size_t id)
{
  auto& callbacks = GetOnFrameCallbacksD3D9();
  return callbacks.Unregister(id);
}
}
}
