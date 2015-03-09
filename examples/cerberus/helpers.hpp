// Copyright (C) 2010-2015 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <windows.h>

#include <hadesmem/detail/alias_cast.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{
namespace cerberus
{
class HelperInterface
{
public:
  virtual ~HelperInterface()
  {
  }

  virtual void DetourFunc(Process const& process,
                          std::wstring const& name,
                          void* interface_ptr,
                          std::size_t index,
                          std::unique_ptr<PatchDetour>& detour,
                          void* detour_fn) = 0;

  template <typename Func>
  void DetourFunc(Process const& process,
                  std::wstring const& name,
                  void* interface_ptr,
                  std::size_t index,
                  std::unique_ptr<PatchDetour>& detour,
                  Func detour_fn)
  {
    HADESMEM_DETAIL_STATIC_ASSERT(::hadesmem::detail::IsFunction<Func>::value ||
                                  std::is_pointer<Func>::value);

    return DetourFunc(process,
                      name,
                      interface_ptr,
                      index,
                      detour,
                      ::hadesmem::detail::AliasCast<void*>(detour_fn));
  }

  virtual void DetourFunc(Process const& process,
                          std::wstring const& name,
                          void* interface_ptr,
                          std::size_t index,
                          std::unique_ptr<PatchInt3>& detour,
                          void* detour_fn) = 0;

  template <typename Func>
  void DetourFunc(Process const& process,
                  std::wstring const& name,
                  void* interface_ptr,
                  std::size_t index,
                  std::unique_ptr<PatchInt3>& detour,
                  Func detour_fn)
  {
    HADESMEM_DETAIL_STATIC_ASSERT(::hadesmem::detail::IsFunction<Func>::value ||
                                  std::is_pointer<Func>::value);

    return DetourFunc(process,
                      name,
                      interface_ptr,
                      index,
                      detour,
                      ::hadesmem::detail::AliasCast<void*>(detour_fn));
  }

  virtual void DetourFunc(Process const& process,
                          HMODULE base,
                          std::string const& name,
                          std::unique_ptr<PatchDetour>& detour,
                          void* detour_fn) = 0;

  template <typename Func>
  void DetourFunc(Process const& process,
                  HMODULE base,
                  std::string const& name,
                  std::unique_ptr<PatchDetour>& detour,
                  Func detour_fn)
  {
    HADESMEM_DETAIL_STATIC_ASSERT(::hadesmem::detail::IsFunction<Func>::value ||
                                  std::is_pointer<Func>::value);

    return DetourFunc(process,
                      base,
                      name,
                      detour,
                      ::hadesmem::detail::AliasCast<void*>(detour_fn));
  }

  virtual void DetourFunc(Process const& process,
                          HMODULE base,
                          std::string const& name,
                          std::unique_ptr<PatchInt3>& detour,
                          void* detour_fn) = 0;

  template <typename Func>
  void DetourFunc(Process const& process,
                  HMODULE base,
                  std::string const& name,
                  std::unique_ptr<PatchInt3>& detour,
                  Func detour_fn)
  {
    HADESMEM_DETAIL_STATIC_ASSERT(::hadesmem::detail::IsFunction<Func>::value ||
                                  std::is_pointer<Func>::value);

    return DetourFunc(process,
                      base,
                      name,
                      detour,
                      ::hadesmem::detail::AliasCast<void*>(detour_fn));
  }

  virtual void DetourFunc(Process const& process,
                          std::string const& name,
                          std::unique_ptr<PatchInt3>& detour,
                          void* orig_fn,
                          void* detour_fn) = 0;

  template <typename OrigFn, typename DetourFn>
  void DetourFunc(Process const& process,
                  std::string const& name,
                  std::unique_ptr<PatchInt3>& detour,
                  OrigFn orig_fn,
                  DetourFn detour_fn)
  {
    HADESMEM_DETAIL_STATIC_ASSERT(
      ::hadesmem::detail::IsFunction<OrigFn>::value ||
      std::is_pointer<OrigFn>::value);
    HADESMEM_DETAIL_STATIC_ASSERT(
      ::hadesmem::detail::IsFunction<DetourFn>::value ||
      std::is_pointer<DetourFn>::value);

    return DetourFunc(process,
                      name,
                      detour,
                      ::hadesmem::detail::AliasCast<void*>(orig_fn),
                      ::hadesmem::detail::AliasCast<void*>(detour_fn));
  }

  virtual void UndetourFunc(std::wstring const& name,
                            std::unique_ptr<PatchDetour>& detour,
                            bool remove) = 0;

  virtual void UndetourFunc(std::wstring const& name,
                            std::unique_ptr<PatchInt3>& detour,
                            bool remove) = 0;

  virtual std::pair<std::size_t, std::size_t> InitializeSupportForModule(
    std::wstring const& module_name_upper,
    std::function<void(HMODULE)> const& detour_func,
    std::function<void(bool)> const& undetour_func,
    std::function<std::pair<void*, SIZE_T>&()> const& get_module_func) = 0;

  virtual bool CommonDetourModule(Process const& process,
                                  std::wstring const& name,
                                  HMODULE& base,
                                  std::pair<void*, SIZE_T>& detoured_mod) = 0;

  virtual bool CommonUndetourModule(std::wstring const& name,
                                    std::pair<void*, SIZE_T>& detoured_mod) = 0;
};

HelperInterface& GetHelperInterface() HADESMEM_DETAIL_NOEXCEPT;
}
}
