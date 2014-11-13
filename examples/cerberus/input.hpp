// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>
#include <functional>

#include <windows.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
typedef void OnWndProcMsgCallback(
  HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool* handled);

typedef void
  OnSetCursorCallback(HCURSOR cursor, bool* handled, HCURSOR* retval);

typedef void OnGetCursorPosCallback(LPPOINT point, bool* handled);

typedef void OnSetCursorPosCallback(int x, int y, bool* handled);

typedef void OnDirectInputCallback(bool* handled);

typedef void OnShowCursorCallback(BOOL show, bool* handled, int* retval);

typedef void
  OnClipCursorCallback(RECT const* rect, bool* handled, BOOL* retval);

typedef void OnGetClipCursorCallback(RECT* rect, bool* handled, BOOL* retval);

class InputInterface
{
public:
  virtual ~InputInterface()
  {
  }

  virtual std::size_t RegisterOnWndProcMsg(
    std::function<OnWndProcMsgCallback> const& callback) = 0;

  virtual void UnregisterOnWndProcMsg(std::size_t id) = 0;

  virtual std::size_t
    RegisterOnSetCursor(std::function<OnSetCursorCallback> const& callback) = 0;

  virtual void UnregisterOnSetCursor(std::size_t id) = 0;

  virtual std::size_t RegisterOnGetCursorPos(
    std::function<OnGetCursorPosCallback> const& callback) = 0;

  virtual void UnregisterOnGetCursorPos(std::size_t id) = 0;

  virtual std::size_t RegisterOnSetCursorPos(
    std::function<OnSetCursorPosCallback> const& callback) = 0;

  virtual void UnregisterOnSetCursorPos(std::size_t id) = 0;

  virtual std::size_t RegisterOnDirectInput(
    std::function<OnDirectInputCallback> const& callback) = 0;

  virtual void UnregisterOnDirectInput(std::size_t id) = 0;

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

InputInterface& GetInputInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeInput();

void DetourDirectInput8(HMODULE base);

void UndetourDirectInput8(bool remove);

void DetourUser32(HMODULE base);

void UndetourUser32(bool remove);

void HandleWindowChange(HWND wnd);

bool IsWindowHooked() HADESMEM_DETAIL_NOEXCEPT;

HWND GetCurrentWindow() HADESMEM_DETAIL_NOEXCEPT;

bool& GetDisableSetCursorHook() HADESMEM_DETAIL_NOEXCEPT;

bool& GetDisableGetCursorPosHook() HADESMEM_DETAIL_NOEXCEPT;

bool& GetDisableShowCursorHook() HADESMEM_DETAIL_NOEXCEPT;

bool& GetDisableClipCursorHook() HADESMEM_DETAIL_NOEXCEPT;

bool& GetDisableGetClipCursorHook() HADESMEM_DETAIL_NOEXCEPT;
}
}
