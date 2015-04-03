// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <windows.h>
#include <hadesmem/detail/warning_disable_prefix.hpp>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>

#include "callbacks.hpp"

namespace hadesmem
{
namespace cerberus
{
enum class DirectInputDeviceType
{
  Mouse,
  Keyboard,
  Other
};

DirectInputDeviceType DeviceGuidToEnum(IDirectInput8A* direct_input,
                                       REFGUID rguid);

DirectInputDeviceType DeviceGuidToEnum(IDirectInput8W* direct_input,
                                       REFGUID rguid);

std::string DeviceTypeToString(DirectInputDeviceType type);

typedef void OnDirectInputCallback(bool* handled);

Callbacks<OnDirectInputCallback>& GetOnDirectInputCallbacks();

class DirectInputInterface
{
public:
  virtual ~DirectInputInterface()
  {
  }

  virtual std::size_t RegisterOnDirectInput(
    std::function<OnDirectInputCallback> const& callback) = 0;

  virtual void UnregisterOnDirectInput(std::size_t id) = 0;
};

DirectInputInterface& GetDirectInputInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeDirectInput();

void DetourDirectInput8(HMODULE base);

void UndetourDirectInput8(bool remove);
}
}
