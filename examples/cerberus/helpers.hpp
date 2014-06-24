// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <windows.h>

#include <hadesmem/patcher.hpp>

namespace hadesmem
{

namespace cerberus
{

void UndetourFunc(std::wstring const& name,
                  std::unique_ptr<hadesmem::PatchDetour>& detour,
                  std::atomic<std::uint32_t>& ref_count,
                  bool remove);

void InitializeSupportForModule(
  std::wstring const& module_name_upper,
  std::function<void(HMODULE)> const& detour_func,
  std::function<void(bool)> const& undetour_func,
  std::function<std::pair<void*, SIZE_T>&()> const& get_module_func);

}
}
