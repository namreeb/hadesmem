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
  HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool& handled);

class InputInterface
{
public:
  virtual ~InputInterface()
  {
  }

  virtual std::size_t RegisterOnWndProcMsg(
    std::function<OnWndProcMsgCallback> const& callback) = 0;

  virtual void UnregisterOnWndProcMsg(std::size_t id) = 0;
};

InputInterface& GetInputInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeInput();

void DetourDirectInput8(HMODULE base);

void UndetourDirectInput8(bool remove);

std::size_t RegisterOnWndProcMsgCallback(
  std::function<OnWndProcMsgCallback> const& callback);

void UnregisterOnWndProcMsgCallback(std::size_t id);

void HandleWindowChange(HWND wnd);

bool IsWindowHooked() HADESMEM_DETAIL_NOEXCEPT;

HWND GetCurrentWindow() HADESMEM_DETAIL_NOEXCEPT;
}
}
