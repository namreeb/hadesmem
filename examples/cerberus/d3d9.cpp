// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d9.hpp"

#include <algorithm>
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

std::unique_ptr<hadesmem::PatchDetour>&
  GetDirect3DCreate9Detour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetDirect3DCreate9ExDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDirect3D9CreateDeviceDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDirect3D9ExCreateDeviceExDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetIDirect3DDevice9EndSceneDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
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

extern "C" HRESULT WINAPI IDirect3DDevice9EndSceneDetour(
  IDirect3DDevice9* device) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDirect3DDevice9EndSceneDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  auto& callbacks = GetOnFrameCallbacksD3D9();
  callbacks.Run(device);

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
  HADESMEM_DETAIL_TRACE_A("Detouring IDirect3DDevice9.");

  try
  {
    auto const& process = hadesmem::cerberus::GetThisProcess();
    hadesmem::cerberus::DetourFunc(process,
                                   L"IDirect3DDevice9::EndScene",
                                   device,
                                   42,
                                   GetIDirect3DDevice9EndSceneDetour(),
                                   IDirect3DDevice9EndSceneDetour);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

extern "C" HRESULT WINAPI IDirect3D9CreateDeviceDetour(
  IDirect3D9* direct3d9,
  UINT adapter,
  D3DDEVTYPE device_type,
  HWND focus_window,
  DWORD behavior_flags,
  D3DPRESENT_PARAMETERS* presentation_params,
  IDirect3DDevice9** returned_device) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDirect3D9CreateDeviceDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%p] [%u] [%d] [%p] [%u] [%p] [%p].",
                                 direct3d9,
                                 adapter,
                                 device_type,
                                 focus_window,
                                 behavior_flags,
                                 presentation_params,
                                 returned_device);
  auto const create_device =
    detour->GetTrampoline<decltype(&IDirect3D9CreateDeviceDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_device(direct3d9,
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

extern "C" HRESULT WINAPI IDirect3D9ExCreateDeviceExDetour(
  IDirect3D9* direct3d9,
  UINT adapter,
  D3DDEVTYPE device_type,
  HWND focus_window,
  DWORD behavior_flags,
  D3DPRESENT_PARAMETERS* presentation_params,
  D3DDISPLAYMODEEX* fullscreen_display_mode,
  IDirect3DDevice9Ex** returned_device) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetIDirect3D9ExCreateDeviceExDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%u] [%d] [%p] [%u] [%p] [%p] [%p].",
    direct3d9,
    adapter,
    device_type,
    focus_window,
    behavior_flags,
    presentation_params,
    fullscreen_display_mode,
    returned_device);
  auto const create_device_ex =
    detour->GetTrampoline<decltype(&IDirect3D9ExCreateDeviceExDetour)>();
  last_error_preserver.Revert();
  auto const ret = create_device_ex(direct3d9,
                                    adapter,
                                    device_type,
                                    focus_window,
                                    behavior_flags,
                                    presentation_params,
                                    fullscreen_display_mode,
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
  HADESMEM_DETAIL_TRACE_A("Detouring IDirect3D9.");

  try
  {
    auto const& process = hadesmem::cerberus::GetThisProcess();
    hadesmem::cerberus::DetourFunc(process,
                                   L"IDirect3D9::CreateDevice",
                                   direct3d9,
                                   16,
                                   GetIDirect3D9CreateDeviceDetour(),
                                   IDirect3D9CreateDeviceDetour);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }
}

void DetourDirect3D9(IDirect3D9Ex* direct3d9)
{
  HADESMEM_DETAIL_TRACE_A("Detouring IDirect3D9Ex.");

  try
  {
    auto const& process = hadesmem::cerberus::GetThisProcess();
    hadesmem::cerberus::DetourFunc(process,
                                   L"IDirect3D9Ex::CreateDeviceEx",
                                   direct3d9,
                                   20,
                                   GetIDirect3D9ExCreateDeviceExDetour(),
                                   IDirect3D9ExCreateDeviceExDetour);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  DetourDirect3D9(static_cast<IDirect3D9*>(direct3d9));
}

extern "C" IDirect3D9* WINAPI
  Direct3DCreate9Detour(UINT sdk_version) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetDirect3DCreate9Detour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%u].", sdk_version);
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

extern "C" HRESULT WINAPI
  Direct3DCreate9ExDetour(UINT sdk_version,
                          IDirect3D9Ex** d3d9_ex) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetDirect3DCreate9ExDetour();
  hadesmem::detail::DetourRefCounter ref_count{detour->GetRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%u] [%p].", sdk_version, d3d9_ex);
  auto const direct3d_create_9_ex =
    detour->GetTrampoline<decltype(&Direct3DCreate9ExDetour)>();
  last_error_preserver.Revert();
  auto const ret = direct3d_create_9_ex(sdk_version, d3d9_ex);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%p].", ret);

  if (SUCCEEDED(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");

    DetourDirect3D9(*d3d9_ex);
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
  InitializeSupportForModule(L"D3D9", DetourD3D9, UndetourD3D9, GetD3D9Module);
}

void DetourD3D9(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetD3D9Module();
  if (CommonDetourModule(process, L"D3D9", base, module))
  {
    DetourFunc(process,
               base,
               "Direct3DCreate9",
               GetDirect3DCreate9Detour(),
               Direct3DCreate9Detour);
    DetourFunc(process,
               base,
               "Direct3DCreate9Ex",
               GetDirect3DCreate9Detour(),
               Direct3DCreate9Detour);
  }
}

void UndetourD3D9(bool remove)
{
  auto& module = GetD3D9Module();
  if (CommonUndetourModule(L"D3D9", module))
  {
    UndetourFunc(L"Direct3DCreate9", GetDirect3DCreate9Detour(), remove);
    UndetourFunc(L"Direct3DCreate9Ex", GetDirect3DCreate9Detour(), remove);
    UndetourFunc(
      L"IDirect3D9::CreateDevice", GetIDirect3D9CreateDeviceDetour(), remove);
    UndetourFunc(L"IDirect3D9Ex::CreateDeviceEx",
                 GetIDirect3D9ExCreateDeviceExDetour(),
                 remove);
    UndetourFunc(L"IDirect3DDevice9::EndScene",
                 GetIDirect3DDevice9EndSceneDetour(),
                 remove);

    module = std::make_pair(nullptr, 0);
  }
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
