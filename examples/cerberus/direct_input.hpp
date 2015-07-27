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

typedef void OnGetDeviceDataCallback(DWORD len_object_data,
                                     LPDIDEVICEOBJECTDATA rgdod,
                                     LPDWORD in_out,
                                     DWORD flags,
                                     HRESULT* retval,
                                     void* device,
                                     bool wide);

typedef void
  OnGetDeviceStateCallback(DWORD len_data, LPVOID data, HRESULT* retval);

Callbacks<OnGetDeviceDataCallback>& GetOnGetDeviceDataCallbacks();

Callbacks<OnGetDeviceStateCallback>& GetOnGetDeviceStateCallbacks();

class DirectInputInterface
{
public:
  virtual ~DirectInputInterface()
  {
  }

  virtual std::size_t RegisterOnGetDeviceData(
    std::function<OnGetDeviceDataCallback> const& callback) = 0;

  virtual void UnregisterOnGetDeviceData(std::size_t id) = 0;

  virtual std::size_t RegisterOnGetDeviceState(
    std::function<OnGetDeviceStateCallback> const& callback) = 0;

  virtual void UnregisterOnGetDeviceState(std::size_t id) = 0;
};

DirectInputInterface& GetDirectInputInterface() noexcept;

void InitializeDirectInput();

void DetourDirectInput8(HMODULE base);

void UndetourDirectInput8(bool remove);
}
}
