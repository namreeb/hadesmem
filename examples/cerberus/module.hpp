// Copyright (C) 2010-2015 Joshua Boyce
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
                           std::wstring const& module_name_upper);

typedef void OnUnmapCallback(HMODULE module);

typedef void OnLoadCallback(HMODULE handle,
                            PCWSTR path,
                            PULONG flags,
                            std::wstring const& full_name,
                            std::wstring const& module_name_upper);

typedef void OnUnloadCallback(HMODULE module);

class ModuleInterface
{
public:
  virtual ~ModuleInterface()
  {
  }

  virtual std::size_t
    RegisterOnMap(std::function<OnMapCallback> const& callback) = 0;

  virtual void UnregisterOnMap(std::size_t id) = 0;

  virtual std::size_t
    RegisterOnUnmap(std::function<OnUnmapCallback> const& callback) = 0;

  virtual void UnregisterOnUnmap(std::size_t id) = 0;

  virtual std::size_t
    RegisterOnLoad(std::function<OnLoadCallback> const& callback) = 0;

  virtual void UnregisterOnLoad(std::size_t id) = 0;

  virtual std::size_t
    RegisterOnUnload(std::function<OnUnloadCallback> const& callback) = 0;

  virtual void UnregisterOnUnload(std::size_t id) = 0;
};

ModuleInterface& GetModuleInterface() noexcept;

void InitializeModule();

void DetourNtdllForModule(HMODULE base);

void UndetourNtdllForModule(bool remove);
}
}
