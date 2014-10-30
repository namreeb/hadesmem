// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <windows.h>

#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/detail/alias_cast.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{
namespace cerberus
{
void DetourFunc(Process const& process,
                std::wstring const& name,
                void* interface_ptr,
                std::size_t index,
                std::unique_ptr<PatchDetour>& detour,
                void* detour_fn);

template <typename Func>
void DetourFunc(Process const& process,
                std::wstring const& name,
                void* interface_ptr,
                std::size_t index,
                std::unique_ptr<PatchDetour>& detour,
                Func detour_fn)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<Func>::value ||
                                std::is_pointer<Func>::value);

  return DetourFunc(process,
                    name,
                    interface_ptr,
                    index,
                    detour,
                    detail::AliasCast<void*>(detour_fn));
}

void DetourFunc(Process const& process,
                HMODULE base,
                std::string const& name,
                std::unique_ptr<PatchDetour>& detour,
                void* detour_fn);

template <typename Func>
void DetourFunc(Process const& process,
                HMODULE base,
                std::string const& name,
                std::unique_ptr<PatchDetour>& detour,
                Func detour_fn)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<Func>::value ||
                                std::is_pointer<Func>::value);

  return DetourFunc(
    process, base, name, detour, detail::AliasCast<void*>(detour_fn));
}

void UndetourFunc(std::wstring const& name,
                  std::unique_ptr<PatchDetour>& detour,
                  bool remove);

void InitializeSupportForModule(
  std::wstring const& module_name_upper,
  std::function<void(HMODULE)> const& detour_func,
  std::function<void(bool)> const& undetour_func,
  std::function<std::pair<void*, SIZE_T>&()> const& get_module_func);

bool CommonDetourModule(Process const& process,
                        std::wstring const& name,
                        HMODULE& base,
                        std::pair<void*, SIZE_T>& detoured_mod);

bool CommonUndetourModule(std::wstring const& name,
                          std::pair<void*, SIZE_T>& detoured_mod);
}
}
