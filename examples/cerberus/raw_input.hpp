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
typedef void OnGetRawInputBufferCallback(
  PRAWINPUT data, PUINT size, UINT size_header, bool* handled, UINT* retval);

typedef void OnGetRawInputDataCallback(HRAWINPUT raw_input,
                                       UINT command,
                                       LPVOID data,
                                       PUINT size,
                                       UINT size_header,
                                       bool* handled,
                                       UINT* retval);

typedef void
  OnRegisterRawInputDevicesCallback(PCRAWINPUTDEVICE raw_input_devices,
                                    UINT num_devices,
                                    UINT size,
                                    bool* handled,
                                    BOOL* retval);

class RawInputInterface
{
public:
  virtual ~RawInputInterface()
  {
  }

  virtual std::size_t RegisterOnGetRawInputBuffer(
    std::function<OnGetRawInputBufferCallback> const& callback) = 0;

  virtual void UnregisterOnGetRawInputBuffer(std::size_t id) = 0;

  virtual std::size_t RegisterOnGetRawInputData(
    std::function<OnGetRawInputDataCallback> const& callback) = 0;

  virtual void UnregisterOnGetRawInputData(std::size_t id) = 0;

  virtual std::size_t RegisterOnRegisterRawInputDevices(
    std::function<OnRegisterRawInputDevicesCallback> const& callback) = 0;

  virtual void UnregisterOnRegisterRawInputDevices(std::size_t id) = 0;
};

RawInputInterface& GetRawInputInterface() noexcept;

void InitializeRawInput();

void DetourUser32ForRawInput(HMODULE base);

void UndetourUser32ForRawInput(bool remove);

bool& GetDisableRegisterRawInputDevicesHook() noexcept;
}
}
