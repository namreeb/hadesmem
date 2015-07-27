// Copyright (C) 2010-2015 Joshua Boyce
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
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{
namespace cerberus
{
template <typename T, typename U>
void DetourFunc(hadesmem::Process const& process,
                std::wstring const& name,
                void* interface_ptr,
                std::size_t index,
                std::unique_ptr<T>& detour,
                U const& detour_fn)
{
  (void)name;
  if (!detour)
  {
    void** const vtable = *reinterpret_cast<void***>(interface_ptr);
    HADESMEM_DETAIL_TRACE_FORMAT_A("VTable: [%p].", vtable);
    auto const target_fn =
      reinterpret_cast<typename T::TargetFuncRawT>(vtable[index]);
    detour = std::make_unique<T>(process, target_fn, detour_fn);
    detour->Apply();
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s detoured.", name.c_str());
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s already detoured.", name.c_str());
  }
}

template <typename T, typename U, typename V>
void DetourFunc(hadesmem::Process const& process,
                std::string const& name,
                std::unique_ptr<T>& detour,
                U const& orig_fn,
                V const& detour_fn)
{
  (void)name;
  if (!detour)
  {
    if (orig_fn)
    {
      detour.reset(new T(process, orig_fn, detour_fn));
      detour->Apply();
      HADESMEM_DETAIL_TRACE_FORMAT_A("%s detoured.", name.c_str());
    }
    else
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A("Could not find %s export.", name.c_str());
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A("%s already detoured.", name.c_str());
  }
}

template <typename T, typename U>
void DetourFunc(hadesmem::Process const& process,
                HMODULE base,
                std::string const& name,
                std::unique_ptr<T>& detour,
                U const& detour_fn)
{
  auto const orig_fn = hadesmem::detail::AliasCast<typename T::TargetFuncRawT>(
    hadesmem::detail::GetProcAddressInternal(process, base, name));
  DetourFunc(process, name, detour, orig_fn, detour_fn);
}

template <typename T>
void UndetourFunc(std::wstring const& name,
                  std::unique_ptr<T>& detour,
                  bool remove)
{
  (void)name;
  if (detour)
  {
    remove ? detour->Remove() : detour->Detach();
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s undetoured.", name.c_str());

    auto& ref_count = detour->GetRefCount();
    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Spinning on %s ref count.",
                                     name.c_str());
    }
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s free of references.", name.c_str());

    detour = nullptr;
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s not detoured. Skipping.", name.c_str());
  }
}

class HelperInterface
{
public:
  virtual ~HelperInterface()
  {
  }

  virtual std::pair<std::size_t, std::size_t> InitializeSupportForModule(
    std::wstring const& module_name_upper,
    std::function<void(HMODULE)> const& detour_func,
    std::function<void(bool)> const& undetour_func,
    std::function<std::pair<void*, SIZE_T>&()> const& get_module_func,
    bool on_map = true) = 0;

  virtual bool CommonDetourModule(Process const& process,
                                  std::wstring const& name,
                                  HMODULE& base,
                                  std::pair<void*, SIZE_T>& detoured_mod) = 0;

  virtual bool CommonUndetourModule(std::wstring const& name,
                                    std::pair<void*, SIZE_T>& detoured_mod) = 0;
};

HelperInterface& GetHelperInterface() noexcept;
}
}
