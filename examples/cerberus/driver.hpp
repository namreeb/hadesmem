// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <windows.h>
#include <winternl.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
typedef void OnNtLoadDriverCallback(PUNICODE_STRING driver_service_name,
                                    bool* handled);

class DriverInterface
{
public:
  virtual ~DriverInterface()
  {
  }

  virtual std::size_t RegisterOnNtLoadDriver(
    std::function<OnNtLoadDriverCallback> const& callback) = 0;

  virtual void UnregisterOnNtLoadDriver(std::size_t id) = 0;
};

DriverInterface& GetDriverInterface() noexcept;

void InitializeDriver();

void DetourNtdllForDriver(HMODULE base);

void UndetourNtdllForDriver(bool remove);
}
}
