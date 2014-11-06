// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "opengl.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

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
class OpenGL32Impl : public hadesmem::cerberus::OpenGL32Interface
{
public:
  virtual std::size_t RegisterOnFrameCallback(
    std::function<hadesmem::cerberus::OnFrameCallbackOpenGL32> const& callback)
    final
  {
    return hadesmem::cerberus::RegisterOnFrameCallbackOpenGL32(callback);
  }

  virtual void UnregisterOnFrameCallback(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnFrameCallbackOpenGL32(id);
  }

  virtual std::size_t RegisterOnUnloadCallback(
    std::function<hadesmem::cerberus::OnUnloadCallbackOpenGL32> const& callback)
    final
  {
    return hadesmem::cerberus::RegisterOnUnloadCallbackOpenGL32(callback);
  }

  virtual void UnregisterOnUnloadCallback(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnUnloadCallbackOpenGL32(id);
  }
};

std::unique_ptr<hadesmem::PatchDetour>&
  GetWglSwapBuffersDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetOpenGL32Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackOpenGL32>&
  GetOnFrameCallbacksOpenGL32()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnFrameCallbackOpenGL32> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnUnloadCallbackOpenGL32>&
  GetOnUnloadCallbacksOpenGL32()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnUnloadCallbackOpenGL32> callbacks;
  return callbacks;
}

extern "C" BOOL WINAPI WglSwapBuffersDetour(HDC device) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetWglSwapBuffersDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  auto& callbacks = GetOnFrameCallbacksOpenGL32();
  callbacks.Run(device);

  auto const end_scene =
    detour->GetTrampoline<decltype(&WglSwapBuffersDetour)>();
  last_error_preserver.Revert();
  auto const ret = end_scene(device);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}
}

namespace hadesmem
{
namespace cerberus
{
OpenGL32Interface& GetOpenGL32Interface() HADESMEM_DETAIL_NOEXCEPT
{
  static OpenGL32Impl impl;
  return impl;
}

void InitializeOpenGL32()
{
  InitializeSupportForModule(
    L"OpenGL32", DetourOpenGL32, UndetourOpenGL32, GetOpenGL32Module);
}

void DetourOpenGL32(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetOpenGL32Module();
  if (CommonDetourModule(process, L"OpenGL32", base, module))
  {
    DetourFunc(process,
               base,
               "wglSwapBuffers",
               GetWglSwapBuffersDetour(),
               WglSwapBuffersDetour);
  }
}

void UndetourOpenGL32(bool remove)
{
  auto& module = GetOpenGL32Module();
  if (CommonUndetourModule(L"OpenGL32", module))
  {
    UndetourFunc(L"wglSwapBuffers", GetWglSwapBuffersDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }

  auto const& callbacks = GetOnUnloadCallbacksOpenGL32();
  callbacks.Run();
}

std::size_t RegisterOnFrameCallbackOpenGL32(
  std::function<OnFrameCallbackOpenGL32> const& callback)
{
  auto& callbacks = GetOnFrameCallbacksOpenGL32();
  return callbacks.Register(callback);
}

void UnregisterOnFrameCallbackOpenGL32(std::size_t id)
{
  auto& callbacks = GetOnFrameCallbacksOpenGL32();
  return callbacks.Unregister(id);
}

std::size_t RegisterOnUnloadCallbackOpenGL32(
  std::function<OnUnloadCallbackOpenGL32> const& callback)
{
  auto& callbacks = GetOnUnloadCallbacksOpenGL32();
  return callbacks.Register(callback);
}

void UnregisterOnUnloadCallbackOpenGL32(std::size_t id)
{
  auto& callbacks = GetOnUnloadCallbacksOpenGL32();
  return callbacks.Unregister(id);
}
}
}
