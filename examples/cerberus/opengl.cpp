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
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameOpenGL32Callback>&
  GetOnFrameOpenGL32Callbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnFrameOpenGL32Callback> callbacks;
  return callbacks;
}

class OpenGL32Impl : public hadesmem::cerberus::OpenGL32Interface
{
public:
  virtual std::size_t RegisterOnFrame(
    std::function<hadesmem::cerberus::OnFrameOpenGL32Callback> const& callback)
    final
  {
    auto& callbacks = GetOnFrameOpenGL32Callbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnFrame(std::size_t id) final
  {
    auto& callbacks = GetOnFrameOpenGL32Callbacks();
    return callbacks.Unregister(id);
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

extern "C" BOOL WINAPI WglSwapBuffersDetour(HDC device) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetWglSwapBuffersDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  auto& callbacks = GetOnFrameOpenGL32Callbacks();
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
}
}
}
