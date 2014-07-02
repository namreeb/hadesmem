// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <functional>
#include <string>

#include <windows.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

namespace cerberus
{

typedef void OnMapCallback(HMODULE module,
                           std::wstring const& path,
                           std::wstring const& name);

typedef void OnUnmapCallback(HMODULE module);

class ModuleInterface
{
public:
  virtual ~ModuleInterface()
  {
  }

  virtual std::size_t
    RegisterOnMapCallback(std::function<OnMapCallback> const& callback) = 0;

  virtual void UnregisterOnMapCallback(std::size_t id) = 0;

  virtual std::size_t
    RegisterOnUnmapCallback(std::function<OnUnmapCallback> const& callback) = 0;

  virtual void UnregisterOnUnmapCallback(std::size_t id) = 0;
};

ModuleInterface& GetModuleInterface() HADESMEM_DETAIL_NOEXCEPT;

void DetourNtMapViewOfSection();

void DetourNtUnmapViewOfSection();

void UndetourNtMapViewOfSection();

void UndetourNtUnmapViewOfSection();

std::size_t RegisterOnMapCallback(std::function<OnMapCallback> const& callback);

void UnregisterOnMapCallback(std::size_t id);

std::size_t
  RegisterOnUnmapCallback(std::function<OnUnmapCallback> const& callback);

void UnregisterOnUnmapCallback(std::size_t id);
}
}
