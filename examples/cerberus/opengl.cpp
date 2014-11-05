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
class OGLImpl : public hadesmem::cerberus::OGLInterface
{
public:
  virtual std::size_t RegisterOnFrameCallback(
    std::function<hadesmem::cerberus::OnFrameCallbackOGL> const& callback) final
  {
    return hadesmem::cerberus::RegisterOnFrameCallbackOGL(callback);
  }

  virtual void UnregisterOnFrameCallback(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnFrameCallbackOGL(id);
  }

  virtual std::size_t RegisterOnUnloadCallback(
    std::function<hadesmem::cerberus::OnUnloadCallbackOGL> const& callback)
    final
  {
    return hadesmem::cerberus::RegisterOnUnloadCallbackOGL(callback);
  }

  virtual void UnregisterOnUnloadCallback(std::size_t id) final
  {
    hadesmem::cerberus::UnregisterOnUnloadCallbackOGL(id);
  }
};

std::unique_ptr<hadesmem::PatchDetour>&
  GetWglSwapBuffersDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetOGLModule() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackOGL>&
  GetOnFrameCallbacksOGL()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnFrameCallbackOGL>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnUnloadCallbackOGL>&
  GetOnUnloadCallbacksOGL()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnUnloadCallbackOGL>
    callbacks;
  return callbacks;
}

extern "C" BOOL WINAPI WglSwapBuffersDetour(HDC device) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetWglSwapBuffersDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", device);

  auto& callbacks = GetOnFrameCallbacksOGL();
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
OGLInterface& GetOGLInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static OGLImpl impl;
  return impl;
}

void InitializeOGL()
{
  InitializeSupportForModule(L"OpenGL32", DetourOGL, UndetourOGL, GetOGLModule);
}

void DetourOGL(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetOGLModule();
  if (CommonDetourModule(process, L"OpenGL32", base, module))
  {
    DetourFunc(process,
               base,
               "wglSwapBuffers",
               GetWglSwapBuffersDetour(),
               WglSwapBuffersDetour);
  }
}

void UndetourOGL(bool remove)
{
  auto& module = GetOGLModule();
  if (CommonUndetourModule(L"OpenGL32", module))
  {
    UndetourFunc(L"wglSwapBuffers", GetWglSwapBuffersDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }

  auto const& callbacks = GetOnUnloadCallbacksOGL();
  callbacks.Run();
}

std::size_t
  RegisterOnFrameCallbackOGL(std::function<OnFrameCallbackOGL> const& callback)
{
  auto& callbacks = GetOnFrameCallbacksOGL();
  return callbacks.Register(callback);
}

void UnregisterOnFrameCallbackOGL(std::size_t id)
{
  auto& callbacks = GetOnFrameCallbacksOGL();
  return callbacks.Unregister(id);
}

std::size_t RegisterOnUnloadCallbackOGL(
  std::function<OnUnloadCallbackOGL> const& callback)
{
  auto& callbacks = GetOnUnloadCallbacksOGL();
  return callbacks.Register(callback);
}

void UnregisterOnUnloadCallbackOGL(std::size_t id)
{
  auto& callbacks = GetOnUnloadCallbacksOGL();
  return callbacks.Unregister(id);
}
}
}
