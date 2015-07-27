// Copyright (C) 2010-2015 Joshua Boyce
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
typedef void OnWndProcMsgCallback(
  HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool* handled);

typedef void OnGetForegroundWindowCallback(bool* handled, HWND* retval);

class WindowInterface
{
public:
  virtual ~WindowInterface()
  {
  }

  virtual std::size_t RegisterOnWndProcMsg(
    std::function<OnWndProcMsgCallback> const& callback) = 0;

  virtual void UnregisterOnWndProcMsg(std::size_t id) = 0;

  virtual std::size_t RegisterOnGetForegroundWindow(
    std::function<OnGetForegroundWindowCallback> const& callback) = 0;

  virtual void UnregisterOnGetForegroundWindow(std::size_t id) = 0;

  virtual HWND GetCurrentWindow() const = 0;
};

WindowInterface& GetWindowInterface() noexcept;

void InitializeWindow();

void DetourUser32ForWindow(HMODULE base);

void UndetourUser32ForWindow(bool remove);

void HandleWindowChange(HWND wnd);

bool IsWindowHooked() noexcept;
}
}
