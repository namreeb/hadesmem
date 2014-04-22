// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "module.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/last_error_preserver.hpp>
#include <hadesmem/detail/recursion_protector.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "d3d11.hpp"
#include "detour_ref_counter.hpp"
#include "main.hpp"

namespace winternl = hadesmem::detail::winternl;

namespace
{

std::unique_ptr<hadesmem::PatchDetour>& GetNtMapViewOfSectionDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetNtMapViewOfSectionRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

std::unique_ptr<hadesmem::PatchDetour>& GetNtUnmapViewOfSectionDetour()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::atomic<std::uint32_t>& GetNtUnmapViewOfSectionRefCount()
  HADESMEM_DETAIL_NOEXCEPT
{
  static std::atomic<std::uint32_t> ref_count;
  return ref_count;
}

extern "C" NTSTATUS WINAPI
  NtMapViewOfSectionDetour(HANDLE section,
                           HANDLE process,
                           PVOID* base,
                           ULONG_PTR zero_bits,
                           SIZE_T commit_size,
                           PLARGE_INTEGER section_offset,
                           PSIZE_T view_size,
                           winternl::SECTION_INHERIT inherit_disposition,
                           ULONG alloc_type,
                           ULONG alloc_protect) HADESMEM_DETAIL_NOEXCEPT
{
  DetourRefCounter ref_count{GetNtMapViewOfSectionRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto& detour = GetNtMapViewOfSectionDetour();
  auto const nt_map_view_of_section =
    detour->GetTrampoline<decltype(&NtMapViewOfSectionDetour)>();
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

#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool in_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool in_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  if (in_hook)
  {
    return ret;
  }

  // Need recursion protection because NtMapViewOfSection is eventually called
  // by a lot of APIs, and we can't really avoid them all.
  hadesmem::detail::RecursionProtector recursion_protector{&in_hook};
  recursion_protector.Set();

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
  }

  DWORD const pid = ::GetProcessId(process);
  if (!pid || pid != ::GetCurrentProcessId())
  {
    HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Unkown or different process [%lu].",
                                         pid);
    return ret;
  }

  HADESMEM_DETAIL_TRACE_NOISY_A("Current process.");

  try
  {
    HADESMEM_DETAIL_TRACE_NOISY_A("Succeeded.");

    hadesmem::Region const region{GetThisProcess(), *base};
    DWORD const region_type = region.GetType();
    if (region_type != MEM_IMAGE)
    {
      HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Not an image. Type given was %lx.",
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
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Path is %s.", path.c_str());

    auto const backslash = path.find_last_of(L'\\');
    std::size_t const name_beg =
      (backslash != std::wstring::npos ? backslash + 1 : 0);
    std::wstring const module_name(std::begin(path) + name_beg, std::end(path));
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Module name is %s.", module_name.c_str());
    std::wstring const module_name_upper =
      hadesmem::detail::ToUpperOrdinal(module_name);

    if (module_name_upper == L"D3D11" || module_name_upper == L"D3D11.DLL")
    {
      HADESMEM_DETAIL_TRACE_A("D3D11 loaded. Applying hooks.");

      DetourD3D11(reinterpret_cast<HMODULE>(*base));
    }
    else if (module_name_upper == L"DXGI" || module_name_upper == L"DXGI.DLL")
    {
      HADESMEM_DETAIL_TRACE_A("DXGI loaded. Applying hooks.");

      DetourDXGI(reinterpret_cast<HMODULE>(*base));
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return ret;
}

extern "C" NTSTATUS WINAPI
  NtUnmapViewOfSectionDetour(HANDLE process, PVOID base)
  HADESMEM_DETAIL_NOEXCEPT
{
  DetourRefCounter ref_count{GetNtUnmapViewOfSectionRefCount()};
  hadesmem::detail::LastErrorPreserver last_error_preserver;

  auto& detour = GetNtUnmapViewOfSectionDetour();
  auto const nt_unmap_view_of_section =
    detour->GetTrampoline<decltype(&NtUnmapViewOfSectionDetour)>();
  last_error_preserver.Revert();
  auto const ret = nt_unmap_view_of_section(process, base);
  last_error_preserver.Update();

#if defined(HADESMEM_GCC) || defined(HADESMEM_CLANG)
  static thread_local bool in_hook = false;
#elif defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  static __declspec(thread) bool in_hook = false;
#else
#error "[HadesMem] Unsupported compiler."
#endif
  if (in_hook)
  {
    return ret;
  }

  hadesmem::detail::RecursionProtector recursion_protector{&in_hook};
  recursion_protector.Set();

  // This has to be after all our recursion checks, rather than before (which
  // would be better) because OutputDebugString calls UnmapViewOfFile when DBWIN
  // is running.
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Args: [%p] [%p].", process, base);
  HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Ret: [%ld].", ret);

  if (!NT_SUCCESS(ret))
  {
    HADESMEM_DETAIL_TRACE_NOISY_A("Failed.");
  }

  DWORD const pid = ::GetProcessId(process);
  if (!pid || pid != ::GetCurrentProcessId())
  {
    HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A("Unkown or different process [%lu].",
                                         pid);
    return ret;
  }

  HADESMEM_DETAIL_TRACE_NOISY_A("Current process.");

  try
  {
    HADESMEM_DETAIL_TRACE_NOISY_A("Succeeded.");

    auto const d3d11_mod = GetD3D11Module();
    if (base >= d3d11_mod.first &&
        base < static_cast<std::uint8_t*>(d3d11_mod.first) + d3d11_mod.second)
    {
      HADESMEM_DETAIL_TRACE_A("D3D11 unloaded. Removing hooks.");

      UndetourD3D11(false);
    }

    auto const dxgi_mod = GetDXGIModule();
    if (base >= dxgi_mod.first &&
        base < static_cast<std::uint8_t*>(dxgi_mod.first) + dxgi_mod.second)
    {
      HADESMEM_DETAIL_TRACE_A("DXGI unloaded. Removing hooks.");

      UndetourDXGI(false);
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return ret;
}
}

void DetourNtMapViewOfSection()
{
  hadesmem::Module const ntdll{GetThisProcess(), L"ntdll.dll"};
  auto const nt_map_view_of_section =
    hadesmem::FindProcedure(GetThisProcess(), ntdll, "NtMapViewOfSection");
  auto const nt_map_view_of_section_ptr =
    hadesmem::detail::UnionCast<void*>(nt_map_view_of_section);
  auto const nt_map_view_of_section_detour =
    hadesmem::detail::UnionCast<void*>(&NtMapViewOfSectionDetour);
  auto& detour = GetNtMapViewOfSectionDetour();
  detour =
    std::make_unique<hadesmem::PatchDetour>(GetThisProcess(),
                                            nt_map_view_of_section_ptr,
                                            nt_map_view_of_section_detour);
  detour->Apply();
  HADESMEM_DETAIL_TRACE_A("NtMapViewOfSection detoured.");
}

void DetourNtUnmapViewOfSection()
{
  hadesmem::Module const ntdll{GetThisProcess(), L"ntdll.dll"};
  auto const nt_unmap_view_of_section =
    hadesmem::FindProcedure(GetThisProcess(), ntdll, "NtUnmapViewOfSection");
  auto const nt_unmap_view_of_section_ptr =
    hadesmem::detail::UnionCast<void*>(nt_unmap_view_of_section);
  auto const nt_unmap_view_of_section_detour =
    hadesmem::detail::UnionCast<void*>(&NtUnmapViewOfSectionDetour);
  auto& detour = GetNtUnmapViewOfSectionDetour();
  detour =
    std::make_unique<hadesmem::PatchDetour>(GetThisProcess(),
                                            nt_unmap_view_of_section_ptr,
                                            nt_unmap_view_of_section_detour);
  detour->Apply();
  HADESMEM_DETAIL_TRACE_A("NtUnmapViewOfSection detoured.");
}

void UndetourNtMapViewOfSection()
{
  auto& detour = GetNtMapViewOfSectionDetour();
  detour->Remove();
  HADESMEM_DETAIL_TRACE_A("NtMapViewOfSection undetoured.");
  detour = nullptr;

  auto& ref_count = GetNtMapViewOfSectionRefCount();
  while (ref_count.load())
  {
    HADESMEM_DETAIL_TRACE_A("Spinning on NtMapViewOfSection ref count.");
  }
  HADESMEM_DETAIL_TRACE_A("NtMapViewOfSection free of references.");
}

void UndetourNtUnmapViewOfSection()
{
  auto& detour = GetNtMapViewOfSectionDetour();
  detour->Remove();
  HADESMEM_DETAIL_TRACE_A("NtUnmapViewOfSection undetoured.");
  detour = nullptr;

  auto& ref_count = GetNtUnmapViewOfSectionRefCount();
  while (ref_count.load())
  {
    HADESMEM_DETAIL_TRACE_A("Spinning on NtUnmapViewOfSection ref count.");
  }
  HADESMEM_DETAIL_TRACE_A("NtUnmapViewOfSection free of references.");
}
