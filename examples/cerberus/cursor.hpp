// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <windows.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
typedef void
  OnSetCursorCallback(HCURSOR cursor, bool* handled, HCURSOR* retval);

typedef void OnGetCursorPosCallback(LPPOINT point, bool* handled);

typedef void OnSetCursorPosCallback(int x, int y, bool* handled);

typedef void OnShowCursorCallback(BOOL show, bool* handled, int* retval);

typedef void
  OnClipCursorCallback(RECT const* rect, bool* handled, BOOL* retval);

typedef void OnGetClipCursorCallback(RECT* rect, bool* handled, BOOL* retval);

class CursorInterface
{
public:
  virtual ~CursorInterface()
  {
  }

  virtual std::size_t
    RegisterOnSetCursor(std::function<OnSetCursorCallback> const& callback) = 0;

  virtual void UnregisterOnSetCursor(std::size_t id) = 0;

  virtual std::size_t RegisterOnGetCursorPos(
    std::function<OnGetCursorPosCallback> const& callback) = 0;

  virtual void UnregisterOnGetCursorPos(std::size_t id) = 0;

  virtual std::size_t RegisterOnSetCursorPos(
    std::function<OnSetCursorPosCallback> const& callback) = 0;

  virtual void UnregisterOnSetCursorPos(std::size_t id) = 0;

  virtual std::size_t RegisterOnShowCursor(
    std::function<OnShowCursorCallback> const& callback) = 0;

  virtual void UnregisterOnShowCursor(std::size_t id) = 0;

  virtual std::size_t RegisterOnClipCursor(
    std::function<OnClipCursorCallback> const& callback) = 0;

  virtual void UnregisterOnClipCursor(std::size_t id) = 0;

  virtual std::size_t RegisterOnGetClipCursor(
    std::function<OnGetClipCursorCallback> const& callback) = 0;

  virtual void UnregisterOnGetClipCursor(std::size_t id) = 0;
};

CursorInterface& GetCursorInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeCursor();

void DetourUser32ForCursor(HMODULE base);

void UndetourUser32ForCursor(bool remove);

bool& GetDisableSetCursorHook() HADESMEM_DETAIL_NOEXCEPT;

bool& GetDisableGetCursorPosHook() HADESMEM_DETAIL_NOEXCEPT;

bool& GetDisableSetCursorPosHook() HADESMEM_DETAIL_NOEXCEPT;

bool& GetDisableShowCursorHook() HADESMEM_DETAIL_NOEXCEPT;

bool& GetDisableClipCursorHook() HADESMEM_DETAIL_NOEXCEPT;

bool& GetDisableGetClipCursorHook() HADESMEM_DETAIL_NOEXCEPT;
}
}
