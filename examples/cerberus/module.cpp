// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "module.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/recursion_protector.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "callbacks.hpp"
#include "helpers.hpp"
#include "main.hpp"

namespace winternl = hadesmem::detail::winternl;

namespace
{
std::pair<void*, SIZE_T>& GetNtdllModule() noexcept
{
  static std::pair<void*, SIZE_T> module{nullptr, 0};
  return module;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnMapCallback>&
  GetOnMapCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnMapCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnUnmapCallback>&
  GetOnUnmapCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnUnmapCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnLoadCallback>&
  GetOnLoadCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnLoadCallback>
    callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnUnloadCallback>&
  GetOnUnloadCallbacks()
{
  static hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnUnloadCallback>
    callbacks;
  return callbacks;
}

class ModuleImpl : public hadesmem::cerberus::ModuleInterface
{
public:
  virtual std::size_t RegisterOnMap(
    std::function<hadesmem::cerberus::OnMapCallback> const& callback) final
  {
    auto& callbacks = GetOnMapCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnMap(std::size_t id) final
  {
    auto& callbacks = GetOnMapCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnUnmap(
    std::function<hadesmem::cerberus::OnUnmapCallback> const& callback) final
  {
    auto& callbacks = GetOnUnmapCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnUnmap(std::size_t id) final
  {
    auto& callbacks = GetOnUnmapCallbacks();
    return callbacks.Unregister(id);
  }

  virtual std::size_t RegisterOnLoad(
    std::function<hadesmem::cerberus::OnLoadCallback> const& callback) final
  {
    auto& callbacks = GetOnLoadCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnLoad(std::size_t id) final
  {
    auto& callbacks = GetOnLoadCallbacks();
    return callbacks.Unregister(id);
  }
  virtual std::size_t RegisterOnUnload(
    std::function<hadesmem::cerberus::OnUnloadCallback> const& callback) final
  {
    auto& callbacks = GetOnUnloadCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnUnload(std::size_t id) final
  {
    auto& callbacks = GetOnUnloadCallbacks();
    return callbacks.Unregister(id);
  }
};

extern "C" NTSTATUS WINAPI
  NtMapViewOfSection(HANDLE section,
                     HANDLE process,
                     PVOID* base,
                     ULONG_PTR zero_bits,
                     SIZE_T commit_size,
                     PLARGE_INTEGER section_offset,
                     PSIZE_T view_size,
                     winternl::SECTION_INHERIT inherit_disposition,
                     ULONG alloc_type,
                     ULONG alloc_protect);

extern "C" NTSTATUS WINAPI NtUnmapViewOfSection(HANDLE process, PVOID base);

extern "C" NTSTATUS WINAPI LdrLoadDll(PCWSTR path,
                                      PULONG flags,
                                      PCUNICODE_STRING name,
                                      PVOID* handle);

extern "C" NTSTATUS WINAPI LdrUnloadDll(PVOID handle);

std::unique_ptr<hadesmem::PatchDetour<decltype(&NtMapViewOfSection)>>&
  GetNtMapViewOfSectionDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&NtMapViewOfSection)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&NtUnmapViewOfSection)>>&
  GetNtUnmapViewOfSectionDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&NtUnmapViewOfSection)>>
    detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&LdrLoadDll)>>&
  GetLdrLoadDllDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&LdrLoadDll)>> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&LdrUnloadDll)>>&
  GetLdrUnloadDllDetour() noexcept
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&LdrUnloadDll)>> detour;
  return detour;
}

extern "C" NTSTATUS WINAPI
  NtMapViewOfSectionDetour(hadesmem::PatchDetourBase* detour,
                           HANDLE section,
                           HANDLE process,
                           PVOID* base,
                           ULONG_PTR zero_bits,
                           SIZE_T commit_size,
                           PLARGE_INTEGER section_offset,
                           PSIZE_T view_size,
                           winternl::SECTION_INHERIT inherit_disposition,
                           ULONG alloc_type,
                           ULONG alloc_protect) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const nt_map_view_of_section =
    detour->GetTrampolineT<decltype(&NtMapViewOfSection)>();
  last_error_preserver.Revert();
  auto const ret = nt_map_view_of_section(section,
                                          process,
                                          base,
                                          zero_bits,
                                          commit_size,
                                          section_offset,
                                          view_size,
                                          inherit_disposition,
                                          alloc_type,
                                          alloc_protect);
  last_error_preserver.Update();

  thread_local static std::int32_t in_hook = 0;
  if (in_hook)
  {
    return ret;
  }

  // Need recursion protection because NtMapViewOfSection is eventually called
  // by a lot of APIs, and we can't really avoid them all.
  hadesmem::detail::RecursionProtector recursion_protector{&in_hook};

  // This has to be after all our recursion checks, rather than before (which
  // would be better) because OutputDebugString calls MapViewOfFile when DBWIN
  // is running.
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%p] [%p] [%Iu] [%Iu] [%p] [%p] [%d] [%u] [%u].",
    section,
    process,
    base,
    zero_bits,
    commit_size,
    section_offset,
    view_size,
    inherit_disposition,
    alloc_type,
    alloc_protect);
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  if (!NT_SUCCESS(ret))
  {
    HADESMEM_DETAIL_TRACE_NOISY_A("Failed.");
    return ret;
  }

  DWORD const pid = ::GetProcessId(process);
  if (!pid || pid != ::GetCurrentProcessId())
  {
    HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
      "Unkown or different process. PID: [%lu].", pid);
    return ret;
  }

  HADESMEM_DETAIL_TRACE_NOISY_A("Current process.");

  try
  {
    HADESMEM_DETAIL_TRACE_NOISY_A("Succeeded.");

    hadesmem::Region const region{hadesmem::cerberus::GetThisProcess(), *base};
    DWORD const region_type = region.GetType();
    if (region_type != MEM_IMAGE)
    {
      HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Not an image. Type: [%lx].",
                                           region_type);
      return ret;
    }

    void* const arbitrary_user_pointer =
      winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer;
    if (!arbitrary_user_pointer)
    {
      HADESMEM_DETAIL_TRACE_NOISY_A("No arbitrary user pointer.");
      return ret;
    }

    std::wstring const path{static_cast<PCWSTR>(arbitrary_user_pointer)};
    HADESMEM_DETAIL_TRACE_NOISY_FORMAT_W(L"Path: [%s].", path.c_str());

    std::wstring const module_name = hadesmem::detail::GetPathBaseName(path);
    HADESMEM_DETAIL_TRACE_NOISY_FORMAT_W(L"Module name: [%s].",
                                         module_name.c_str());
    std::wstring const module_name_upper =
      hadesmem::detail::ToUpperOrdinal(module_name);

    auto& callbacks = GetOnMapCallbacks();
    callbacks.Run(reinterpret_cast<HMODULE>(*base), path, module_name_upper);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return ret;
}

extern "C" NTSTATUS WINAPI NtUnmapViewOfSectionDetour(
  hadesmem::PatchDetourBase* detour, HANDLE process, PVOID base) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const nt_unmap_view_of_section =
    detour->GetTrampolineT<decltype(&NtUnmapViewOfSection)>();
  last_error_preserver.Revert();
  auto const ret = nt_unmap_view_of_section(process, base);
  last_error_preserver.Update();

  thread_local static std::int32_t in_hook = 0;
  if (in_hook)
  {
    return ret;
  }

  hadesmem::detail::RecursionProtector recursion_protector{&in_hook};

  // This has to be after all our recursion checks, rather than before (which
  // would be better) because OutputDebugString calls UnmapViewOfFile when DBWIN
  // is running.
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%p].", process, base);
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  if (!NT_SUCCESS(ret))
  {
    HADESMEM_DETAIL_TRACE_NOISY_A("Failed.");
    return ret;
  }

  DWORD const pid = ::GetProcessId(process);
  if (!pid || pid != ::GetCurrentProcessId())
  {
    HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Unkown or different process [%lu].",
                                         pid);
    return ret;
  }

  HADESMEM_DETAIL_TRACE_NOISY_A("Succeeded. Current process.");

  auto& callbacks = GetOnUnmapCallbacks();
  callbacks.Run(reinterpret_cast<HMODULE>(base));

  return ret;
}

extern "C" NTSTATUS WINAPI LdrLoadDllDetour(hadesmem::PatchDetourBase* detour,
                                            PCWSTR path,
                                            PULONG flags,
                                            PCUNICODE_STRING name,
                                            PVOID* handle) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const ldr_load_dll = detour->GetTrampolineT<decltype(&LdrLoadDll)>();
  last_error_preserver.Revert();
  auto const ret = ldr_load_dll(path, flags, name, handle);
  last_error_preserver.Update();

  thread_local static std::int32_t in_hook = 0;
  if (in_hook)
  {
    return ret;
  }

  hadesmem::detail::RecursionProtector recursion_protector{&in_hook};

  // This has to be after all our recursion checks, rather than before (which
  // would be better) because we may cause another DLL to load (and/or unload).
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
    "Args: [%p] [%p] [%p] [%p].", path, flags, name, handle);
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  std::uintptr_t const kLazyPath = 0x1;
  if (path && !(reinterpret_cast<std::uintptr_t>(path) & kLazyPath))
  {
    HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(L"Path: [%s]", path);
  }

  if (flags)
  {
    HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Flags: [%lu]", *flags);
  }

  std::wstring const full_name = hadesmem::detail::UnicodeStringToStdString(name);
  std::wstring const module_name = hadesmem::detail::GetPathBaseName(full_name);
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_W(L"Full Name: [%s]. Module Name: [%s].",
                                       full_name.c_str(),
                                       module_name.c_str());
  std::wstring const module_name_upper =
    hadesmem::detail::ToUpperOrdinal(module_name);

  if (!NT_SUCCESS(ret))
  {
    HADESMEM_DETAIL_TRACE_NOISY_A("Failed.");
    return ret;
  }

  HADESMEM_DETAIL_TRACE_NOISY_A("Succeeded.");

  auto& callbacks = GetOnLoadCallbacks();
  callbacks.Run(reinterpret_cast<HMODULE>(*handle),
                path,
                flags,
                full_name,
                module_name_upper);

  return ret;
}

extern "C" NTSTATUS WINAPI LdrUnloadDllDetour(hadesmem::PatchDetourBase* detour,
                                              PVOID handle) noexcept
{
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto const ldr_unload_dll = detour->GetTrampolineT<decltype(&LdrUnloadDll)>();
  last_error_preserver.Revert();
  auto const ret = ldr_unload_dll(handle);
  last_error_preserver.Update();

  thread_local static std::int32_t in_hook = 0;
  if (in_hook)
  {
    return ret;
  }

  hadesmem::detail::RecursionProtector recursion_protector{&in_hook};

  // This has to be after all our recursion checks, rather than before (which
  // would be better) because we may cause another DLL to load (and/or unload).
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p].", handle);
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  if (!NT_SUCCESS(ret))
  {
    HADESMEM_DETAIL_TRACE_NOISY_A("Failed.");
    return ret;
  }

  HADESMEM_DETAIL_TRACE_NOISY_A("Succeeded.");

  auto& callbacks = GetOnUnloadCallbacks();
  callbacks.Run(reinterpret_cast<HMODULE>(handle));

  return ret;
}
}

namespace hadesmem
{
namespace cerberus
{
ModuleInterface& GetModuleInterface() noexcept
{
  static ModuleImpl module_impl;
  return module_impl;
}

void InitializeModule()
{
  auto& helper = GetHelperInterface();
  helper.InitializeSupportForModule(
    L"NTDLL", DetourNtdllForModule, UndetourNtdllForModule, GetNtdllModule);
}

void DetourNtdllForModule(HMODULE base)
{
  auto const& process = GetThisProcess();
  auto& module = GetNtdllModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonDetourModule(process, L"ntdll", base, module))
  {
    DetourFunc(process,
               base,
               "NtMapViewOfSection",
               GetNtMapViewOfSectionDetour(),
               NtMapViewOfSectionDetour);
    DetourFunc(process,
               base,
               "NtUnmapViewOfSection",
               GetNtUnmapViewOfSectionDetour(),
               NtUnmapViewOfSectionDetour);
    DetourFunc(
      process, base, "LdrLoadDll", GetLdrLoadDllDetour(), LdrLoadDllDetour);
    DetourFunc(process,
               base,
               "LdrUnloadDll",
               GetLdrUnloadDllDetour(),
               LdrUnloadDllDetour);
  }
}

void UndetourNtdllForModule(bool remove)
{
  auto& module = GetNtdllModule();
  auto& helper = GetHelperInterface();
  if (helper.CommonUndetourModule(L"ntdll", module))
  {
    UndetourFunc(L"NtMapViewOfSection", GetNtMapViewOfSectionDetour(), remove);
    UndetourFunc(
      L"NtUnmapViewOfSection", GetNtUnmapViewOfSectionDetour(), remove);
    UndetourFunc(L"LdrLoadDll", GetLdrLoadDllDetour(), remove);
    UndetourFunc(L"LdrUnloadDll", GetLdrUnloadDllDetour(), remove);

    module = std::make_pair(nullptr, 0);
  }
}
}
}
