// Copyright (C) 2010-2015 Joshua Boyce.
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

class WindowInterface
{
public:
  virtual ~WindowInterface()
  {
  }

  virtual std::size_t RegisterOnWndProcMsg(
    std::function<OnWndProcMsgCallback> const& callback) = 0;

  virtual void UnregisterOnWndProcMsg(std::size_t id) = 0;

  virtual HWND GetCurrentWindow() const = 0;

  virtual void EnableForegroundWindowSpoof() = 0;

  virtual void DisableForegroundWindowSpoof() = 0;
};

WindowInterface& GetWindowInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeWindow();

void DetourUser32ForWindow(HMODULE base);

void UndetourUser32ForWindow(bool remove);

void HandleWindowChange(HWND wnd);

bool IsWindowHooked() HADESMEM_DETAIL_NOEXCEPT;
}
}
