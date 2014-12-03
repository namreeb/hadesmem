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
typedef void OnSetGuiVisibility(bool visibility, bool old_visibility);

typedef void OnInputQueueEntry(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

class InputInterface
{
public:
  virtual ~InputInterface()
  {
  }

  virtual std::size_t RegisterOnSetGuiVisibility(
    std::function<OnSetGuiVisibility> const& callback) = 0;

  virtual void UnregisterOnSetGuiVisibility(std::size_t id) = 0;

  virtual std::size_t RegisterOnInputQueueEntry(
    std::function<OnInputQueueEntry> const& callback) = 0;

  virtual void UnregisterOnInputQueueEntry(std::size_t id) = 0;
};

bool& GetGuiVisible() HADESMEM_DETAIL_NOEXCEPT;

void SetGuiVisible(bool visible, bool old_visible);

void HandleInputQueue();

InputInterface& GetInputInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeInput();
}
}
