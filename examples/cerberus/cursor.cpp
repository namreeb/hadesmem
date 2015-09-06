// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "cursor.hpp"

#include <cstdint>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/trace.hpp>
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

std::unique_ptr<hadesmem::PatchDetour<decltype(&::SetCursor)>>&
  GetSetCursorDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::SetCursor)>> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::GetCursorPos)>>&
  GetGetCursorPosDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::GetCursorPos)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::SetCursorPos)>>&
  GetSetCursorPosDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::SetCursorPos)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::GetPhysicalCursorPos)>>&
  GetGetPhysicalCursorPosDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::GetCursorPos)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::SetPhysicalCursorPos)>>&
  GetSetPhysicalCursorPosDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::SetCursorPos)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::ShowCursor)>>&
  GetShowCursorDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::ShowCursor)>> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::ClipCursor)>>&
  GetClipCursorDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::ClipCursor)>> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&::GetClipCursor)>>&
  GetGetClipCursorDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&::GetClipCursor)>>
    detour;
  return detour;
}

std::pair<void*, SIZE_T>& GetUser32Module() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

extern "C" HCURSOR WINAPI SetCursorDetour(hadesmem::PatchDetourBase* detour,
                                          HCURSOR cursor) noexcept
{
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

  auto const set_cursor = detour->GetTrampolineT<decltype(&::SetCursor)>();
  last_error_preserver.Revert();
  auto const ret = set_cursor(cursor);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%p].", ret);

  return ret;
}

extern "C" BOOL WINAPI GetCursorPosDetour(hadesmem::PatchDetourBase* detour,
                                          LPPOINT point) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", point);

  if (!hadesmem::cerberus::GetDisableGetCursorPosHook())
  {
    auto const& callbacks = GetOnGetCursorPosCallbacks();
    bool handled = false;
    callbacks.Run(point, false, &handled);

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
    detour->GetTrampolineT<decltype(&::GetCursorPos)>();
  last_error_preserver.Revert();
  auto const ret = get_cursor_pos(point);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

extern "C" BOOL WINAPI SetCursorPosDetour(hadesmem::PatchDetourBase* detour,
                                          int x,
                                          int y) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%d] [%d].", x, y);

  if (!hadesmem::cerberus::GetDisableSetCursorPosHook())
  {
    auto const& callbacks = GetOnSetCursorPosCallbacks();
    bool handled = false;
    callbacks.Run(x, y, false, &handled);

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
    detour->GetTrampolineT<decltype(&::SetCursorPos)>();
  last_error_preserver.Revert();
  auto const ret = set_cursor_pos(x, y);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

extern "C" BOOL WINAPI GetPhysicalCursorPosDetour(
  hadesmem::PatchDetourBase* detour, LPPOINT point) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", point);

  if (!hadesmem::cerberus::GetDisableGetCursorPosHook())
  {
    auto const& callbacks = GetOnGetCursorPosCallbacks();
    bool handled = false;
    callbacks.Run(point, true, &handled);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_NOISY_A(
        "GetPhysicalCursorPos handled. Not calling trampoline.");
      return TRUE;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "GetPhysicalCursorPos hook disabled, skipping callbacks.");
  }

  auto const get_physical_cursor_pos =
    detour->GetTrampolineT<decltype(&::GetPhysicalCursorPos)>();
  last_error_preserver.Revert();
  auto const ret = get_physical_cursor_pos(point);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

extern "C" BOOL WINAPI SetPhysicalCursorPosDetour(
  hadesmem::PatchDetourBase* detour, int x, int y) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%d] [%d].", x, y);

  if (!hadesmem::cerberus::GetDisableSetCursorPosHook())
  {
    auto const& callbacks = GetOnSetCursorPosCallbacks();
    bool handled = false;
    callbacks.Run(x, y, true, &handled);

    if (handled)
    {
      HADESMEM_DETAIL_TRACE_NOISY_A(
        "SetPhysicalCursorPos handled. Not calling trampoline.");
      return TRUE;
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_NOISY_A(
      "SetPhysicalCursorPos hook disabled, skipping callbacks.");
  }

  auto const set_physical_cursor_pos =
    detour->GetTrampolineT<decltype(&::SetCursorPos)>();
  last_error_preserver.Revert();
  auto const ret = set_physical_cursor_pos(x, y);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

extern "C" int WINAPI ShowCursorDetour(hadesmem::PatchDetourBase* detour,
                                       BOOL show) noexcept
{
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

  auto const show_cursor = detour->GetTrampolineT<decltype(&::ShowCursor)>();
  last_error_preserver.Revert();
  auto const ret = show_cursor(show);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

extern "C" BOOL WINAPI ClipCursorDetour(hadesmem::PatchDetourBase* detour,
                                        RECT const* rect) noexcept
{
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

  auto const clip_cursor = detour->GetTrampolineT<decltype(&::ClipCursor)>();
  last_error_preserver.Revert();
  auto const ret = clip_cursor(rect);
  last_error_preserver.Update();
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%d].", ret);

  return ret;
}

extern "C" BOOL WINAPI GetClipCursorDetour_(hadesmem::PatchDetourBase* detour,
                                            RECT* rect) noexcept
{
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
    detour->GetTrampolineT<decltype(&::GetClipCursor)>();
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
CursorInterface& GetCursorInterface() noexcept
{
  static CursorImpl cursor_impl;
  return cursor_impl;
}

void InitializeCursor()
{
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(
    L"USER32", DetourUser32ForCursor, UndetourUser32ForCursor, GetUser32Module);
}

void DetourUser32ForCursor(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetUser32Module();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"user32", base, module))
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
    DetourFunc(process,
               base,
               "GetPhysicalCursorPos",
               GetGetPhysicalCursorPosDetour(),
               GetPhysicalCursorPosDetour);
    DetourFunc(process,
               base,
               "SetPhysicalCursorPos",
               GetSetPhysicalCursorPosDetour(),
               SetPhysicalCursorPosDetour);
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
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"user32", module))
  {
    UndetourFunc(L"SetCursor", GetSetCursorDetour(), remove);
    UndetourFunc(L"GetCursorPos", GetGetCursorPosDetour(), remove);
    UndetourFunc(L"SetCursorPos", GetSetCursorPosDetour(), remove);
    UndetourFunc(
      L"GetPhysicalCursorPos", GetGetPhysicalCursorPosDetour(), remove);
    UndetourFunc(
      L"SetPhysicalCursorPos", GetSetPhysicalCursorPosDetour(), remove);
    UndetourFunc(L"ShowCursor", GetShowCursorDetour(), remove);
    UndetourFunc(L"ClipCursor", GetClipCursorDetour(), remove);
    UndetourFunc(L"GetClipCursor", GetGetClipCursorDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}

bool& GetDisableSetCursorHook() noexcept
{
  thread_local static bool disable_hook = false;
  return disable_hook;
}

bool& GetDisableGetCursorPosHook() noexcept
{
  thread_local static bool disable_hook = false;
  return disable_hook;
}

bool& GetDisableSetCursorPosHook() noexcept
{
  thread_local static bool disable_hook = false;
  return disable_hook;
}

bool& GetDisableShowCursorHook() noexcept
{
  thread_local static bool disable_hook = false;
  return disable_hook;
}

bool& GetDisableClipCursorHook() noexcept
{
  thread_local static bool disable_hook = false;
  return disable_hook;
}

bool& GetDisableGetClipCursorHook() noexcept
{
  thread_local static bool disable_hook = false;
  return disable_hook;
}
}
}
