// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
typedef void
  OnInputQueueEntry(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

class InputInterface
{
public:
  virtual ~InputInterface()
  {
  }

  virtual std::size_t RegisterOnInputQueueEntry(
    std::function<OnInputQueueEntry> const& callback) = 0;

  virtual void UnregisterOnInputQueueEntry(std::size_t id) = 0;
};

void SetGuiVisibleForInput(bool visible, bool old_visible);

void HandleInputQueue();

InputInterface& GetInputInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeInput();
}
}
