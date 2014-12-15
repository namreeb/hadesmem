// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "d3d9.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
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
#include "direct_3d_9.hpp"
#include "helpers.hpp"
#include "main.hpp"
#include "module.hpp"

namespace
{
class D3D9Impl : public hadesmem::cerberus::D3D9Interface
{
public:
  virtual std::size_t RegisterOnFrame(
    std::function<hadesmem::cerberus::OnFrameD3D9Callback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnFrameD3D9Callbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnFrame(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnFrameD3D9Callbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnReset(
    std::function<hadesmem::cerberus::OnResetD3D9Callback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnResetD3D9Callbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnReset(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnResetD3D9Callbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnRelease(
    std::function<hadesmem::cerberus::OnReleaseD3D9Callback> const& callback)
    final
  {
    auto& callbacks = hadesmem::cerberus::GetOnReleaseD3D9Callbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnRelease(std::size_t id) final
  {
    auto& callbacks = hadesmem::cerberus::GetOnReleaseD3D9Callbacks();
    return callbacks.Unregister(id);
  }
};

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

std::pair<void*, SIZE_T>& GetD3D9Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{};
  return module;
}

extern "C" IDirect3D9* WINAPI
  Direct3DCreate9Detour(UINT sdk_version) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetDirect3DCreate9Detour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_FORMAT_A("Args: [%u].", sdk_version);

  auto const direct3d_create_9 =
    detour->GetTrampoline<decltype(&Direct3DCreate9Detour)>();
  last_error_preserver.Revert();
  auto ret = direct3d_create_9(sdk_version);
  last_error_preserver.Update();

  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%p].", ret);

  if (ret)
  {
    HADESMEM_DETAIL_TRACE_A("Succeeded.");
    HADESMEM_DETAIL_TRACE_A("Proxying IDirect3D9.");
    ret = new hadesmem::cerberus::Direct3D9Proxy(ret);
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
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
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
    HADESMEM_DETAIL_TRACE_A("Proxying IDirect3D9Ex.");
    *d3d9_ex = new hadesmem::cerberus::Direct3D9Proxy(*d3d9_ex);
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
Callbacks<OnFrameD3D9Callback>& GetOnFrameD3D9Callbacks()
{
  static Callbacks<OnFrameD3D9Callback> callbacks;
  return callbacks;
}

Callbacks<OnResetD3D9Callback>& GetOnResetD3D9Callbacks()
{
  static Callbacks<OnResetD3D9Callback> callbacks;
  return callbacks;
}

Callbacks<OnReleaseD3D9Callback>& GetOnReleaseD3D9Callbacks()
{
  static Callbacks<OnReleaseD3D9Callback> callbacks;
  return callbacks;
}

D3D9Interface& GetD3D9Interface() HADESMEM_DETAIL_NOEXCEPT
{
  static D3D9Impl d3d9_impl;
  return d3d9_impl;
}

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
               GetDirect3DCreate9ExDetour(),
               Direct3DCreate9ExDetour);
  }
}

void UndetourD3D9(bool remove)
{
  auto& module = GetD3D9Module();
  if (CommonUndetourModule(L"D3D9", module))
  {
    UndetourFunc(L"Direct3DCreate9", GetDirect3DCreate9Detour(), remove);
    UndetourFunc(L"Direct3DCreate9Ex", GetDirect3DCreate9ExDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}
}
}
