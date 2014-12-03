// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "cursor.hpp"

#include <cstdint>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/detour_ref_counter.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/patcher.hpp>

#include "callbacks.hpp"
#include "helpers.hpp"
#include "main.hpp"

namespace
{
hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnSetCursorCallback>&
  GetOnSetCursorCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnSetCursorCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnGetCursorPosCallback>&
  GetOnGetCursorPosCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnGetCursorPosCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnSetCursorPosCallback>&
  GetOnSetCursorPosCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnSetCursorPosCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnShowCursorCallback>&
  GetOnShowCursorCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnShowCursorCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnClipCursorCallback>&
  GetOnClipCursorCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnClipCursorCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnGetClipCursorCallback>&
  GetOnGetClipCursorCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnGetClipCursorCallback> callbacks;
  return callbacks;
}

class CursorImpl : public hadesmem::cerberus::CursorInterface
{
public:
  virtual std::size_t RegisterOnSetCursor(
    std::function<hadesmem::cerberus::OnSetCursorCallback> const& callback)
    final
  {
    auto& callbacks = GetOnSetCursorCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnSetCursor(std::size_t id) final
  {
    auto& callbacks = GetOnSetCursorCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnGetCursorPos(
    std::function<hadesmem::cerberus::OnGetCursorPosCallback> const& callback)
    final
  {
    auto& callbacks = GetOnGetCursorPosCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnGetCursorPos(std::size_t id) final
  {
    auto& callbacks = GetOnGetCursorPosCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnSetCursorPos(
    std::function<hadesmem::cerberus::OnSetCursorPosCallback> const& callback)
    final
  {
    auto& callbacks = GetOnSetCursorPosCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnSetCursorPos(std::size_t id) final
  {
    auto& callbacks = GetOnSetCursorPosCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnShowCursor(
    std::function<hadesmem::cerberus::OnShowCursorCallback> const& callback)
    final
  {
    auto& callbacks = GetOnShowCursorCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnShowCursor(std::size_t id) final
  {
    auto& callbacks = GetOnShowCursorCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnClipCursor(
    std::function<hadesmem::cerberus::OnClipCursorCallback> const& callback)
    final
  {
    auto& callbacks = GetOnClipCursorCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnClipCursor(std::size_t id) final
  {
    auto& callbacks = GetOnClipCursorCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnGetClipCursor(
    std::function<hadesmem::cerberus::OnGetClipCursorCallback> const& callback)
    final
  {
    auto& callbacks = GetOnGetClipCursorCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnGetClipCursor(std::size_t id) final
  {
    auto& callbacks = GetOnGetClipCursorCallbacks();
    return callbacks.Unregister(id);
  }
};

std::unique_ptr<hadesmem::PatchDetour>&
  GetSetCursorDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetGetCursorPosDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetSetCursorPosDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetShowCursorDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetClipCursorDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>&
  GetGetClipCursorDetour() HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetUser32Module() HADESMEM_DETAIL_NOEXCEPT
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

extern "C" HCURSOR WINAPI
  SetCursorDetour(HCURSOR cursor) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetSetCursorDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", cursor);

  if (!hadesmem::cerberus::GetDisableSetCursorHook())
  {
    auto const& callbacks = GetOnSetCursorCallbacks();
    bool handled = false;
    HCURSOR retval{};
    callbacks.Run(cursor, &handled, &retval);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_NOISY_A(
        "SetCursor handled. Not calling trampoline.");
      HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", retval);
      return retval;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "SetCursor hook disabled, skipping callbacks.");
  }

  auto const set_cursor = detour->GetTrampoline<decltype(&SetCursorDetour)>();
  last_error_preserver.Revert();
  auto const ret = set_cursor(cursor);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
}

extern "C" BOOL WINAPI
  GetCursorPosDetour(LPPOINT point) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetGetCursorPosDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", point);

  if (!hadesmem::cerberus::GetDisableGetCursorPosHook())
  {
    auto const& callbacks = GetOnGetCursorPosCallbacks();
    bool handled = false;
    callbacks.Run(point, &handled);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_NOISY_A(
        "GetCursorPos handled. Not calling trampoline.");
      return TRUE;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "GetCursorPos hook disabled, skipping callbacks.");
  }

  auto const get_cursor_pos =
    detour->GetTrampoline<decltype(&GetCursorPosDetour)>();
  last_error_preserver.Revert();
  auto const ret = get_cursor_pos(point);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

extern "C" BOOL WINAPI SetCursorPosDetour(int x, int y) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetSetCursorPosDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%d] [%d].", x, y);

  if (!hadesmem::cerberus::GetDisableSetCursorPosHook())
  {
    auto const& callbacks = GetOnSetCursorPosCallbacks();
    bool handled = false;
    callbacks.Run(x, y, &handled);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_NOISY_A(
        "SetCursorPos handled. Not calling trampoline.");
      return TRUE;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "SetCursorPos hook disabled, skipping callbacks.");
  }

  auto const set_cursor_pos =
    detour->GetTrampoline<decltype(&SetCursorPosDetour)>();
  last_error_preserver.Revert();
  auto const ret = set_cursor_pos(x, y);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

extern "C" int WINAPI ShowCursorDetour(BOOL show) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetShowCursorDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%d].", show);

  if (!hadesmem::cerberus::GetDisableShowCursorHook())
  {
    auto const& callbacks = GetOnShowCursorCallbacks();
    bool handled = false;
    int retval{};
    callbacks.Run(show, &handled, &retval);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_NOISY_A(
        "ShowCursor handled. Not calling trampoline.");
      HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", retval);
      return retval;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "ShowCursor hook disabled, skipping callbacks.");
  }

  auto const show_cursor = detour->GetTrampoline<decltype(&ShowCursorDetour)>();
  last_error_preserver.Revert();
  auto const ret = show_cursor(show);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

extern "C" BOOL WINAPI
  ClipCursorDetour(RECT const* rect) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetClipCursorDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", rect);

  if (!hadesmem::cerberus::GetDisableClipCursorHook())
  {
    auto const& callbacks = GetOnClipCursorCallbacks();
    bool handled = false;
    BOOL retval{};
    callbacks.Run(rect, &handled, &retval);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_NOISY_A(
        "ClipCursor handled. Not calling trampoline.");
      HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", retval);
      return retval;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "ClipCursor hook disabled, skipping callbacks.");
  }

  auto const clip_cursor = detour->GetTrampoline<decltype(&ClipCursorDetour)>();
  last_error_preserver.Revert();
  auto const ret = clip_cursor(rect);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

extern "C" BOOL WINAPI GetClipCursorDetour_(RECT* rect) HADESMEM_DETAIL_NOEXCEPT
{
  auto& detour = GetGetClipCursorDetour();
  auto const ref_counter =
    hadesmem::detail::MakeDetourRefCounter(detour->GetRefCount());
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", rect);

  if (!hadesmem::cerberus::GetDisableGetClipCursorHook())
  {
    auto const& callbacks = GetOnGetClipCursorCallbacks();
    bool handled = false;
    BOOL retval{};
    callbacks.Run(rect, &handled, &retval);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_NOISY_A(
        "GetClipCursor handled. Not calling trampoline.");
      HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", retval);
      return retval;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "GetClipCursor hook disabled, skipping callbacks.");
  }

  auto const get_clip_cursor =
    detour->GetTrampoline<decltype(&GetClipCursorDetour_)>();
  last_error_preserver.Revert();
  auto const ret = get_clip_cursor(rect);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}
}

namespace hadesmem
{
namespace cerberus
{
CursorInterface& GetCursorInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static CursorImpl cursor_impl;
  return cursor_impl;
}

void InitializeCursor()
{
  InitializeSupportForModule(
    L"user32", DetourUser32ForCursor, UndetourUser32ForCursor, GetUser32Module);
}

void DetourUser32ForCursor(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetUser32Module();
  if (CommonDetourModule(process, L"user32", base, module))
  {
    DetourFunc(
      process, base, "SetCursor", GetSetCursorDetour(), SetCursorDetour);
    DetourFunc(process,
               base,
               "GetCursorPos",
               GetGetCursorPosDetour(),
               GetCursorPosDetour);
    DetourFunc(process,
               base,
               "SetCursorPos",
               GetSetCursorPosDetour(),
               SetCursorPosDetour);
    DetourFunc(
      process, base, "ShowCursor", GetShowCursorDetour(), ShowCursorDetour);
    DetourFunc(
      process, base, "ClipCursor", GetClipCursorDetour(), ClipCursorDetour);
    DetourFunc(process,
               base,
               "GetClipCursor",
               GetGetClipCursorDetour(),
               GetClipCursorDetour_);
  }
}

void UndetourUser32ForCursor(bool remove)
{
  auto& module = GetUser32Module();
  if (CommonUndetourModule(L"user32", module))
  {
    UndetourFunc(L"SetCursor", GetSetCursorDetour(), remove);
    UndetourFunc(L"GetCursorPos", GetGetCursorPosDetour(), remove);
    UndetourFunc(L"SetCursorPos", GetSetCursorPosDetour(), remove);
    UndetourFunc(L"ShowCursor", GetShowCursorDetour(), remove);
    UndetourFunc(L"ClipCursor", GetClipCursorDetour(), remove);
    UndetourFunc(L"GetClipCursor", GetGetClipCursorDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}

bool& GetDisableSetCursorHook() HADESMEM_DETAIL_NOEXCEPT
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool disable_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool disable_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  return disable_hook;
}

bool& GetDisableGetCursorPosHook() HADESMEM_DETAIL_NOEXCEPT
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool disable_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool disable_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  return disable_hook;
}

bool& GetDisableSetCursorPosHook() HADESMEM_DETAIL_NOEXCEPT
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool disable_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool disable_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  return disable_hook;
}

bool& GetDisableShowCursorHook() HADESMEM_DETAIL_NOEXCEPT
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool disable_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool disable_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  return disable_hook;
}

bool& GetDisableClipCursorHook() HADESMEM_DETAIL_NOEXCEPT
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool disable_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool disable_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  return disable_hook;
}

bool& GetDisableGetClipCursorHook() HADESMEM_DETAIL_NOEXCEPT
{
#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool disable_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool disable_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  return disable_hook;
}
}
}
