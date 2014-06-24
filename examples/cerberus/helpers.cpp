// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "helpers.hpp"

#include <hadesmem/detail/trace.hpp>

#include "module.hpp"

namespace hadesmem
{

namespace cerberus
{

void UndetourFunc(std::wstring const& name,
                  std::unique_ptr<hadesmem::PatchDetour>& detour,
                  std::atomic<std::uint32_t>& ref_count,
                  bool remove)
{
  if (detour)
  {
    remove ? detour->Remove() : detour->Detach();
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s undetoured.", name.c_str());
    detour = nullptr;

    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Spinning on %s ref count.",
                                     name.c_str());
    }
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s free of references.", name.c_str());
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s not detoured. Skipping.", name.c_str());
  }
}

void InitializeSupportForModule(
  std::wstring const& module_name_upper,
  std::function<void(HMODULE)> const& detour_func,
  std::function<void(bool)> const& undetour_func,
  std::function<std::pair<void*, SIZE_T>&()> const& get_module_func)
{
  HADESMEM_DETAIL_TRACE_FORMAT_W(L"Initializing %s support.",
                                 module_name_upper.c_str());

  std::wstring const module_name_upper_with_ext{module_name_upper + L".DLL"};
  auto const on_map = [=](
    HMODULE module, std::wstring const& /*path*/, std::wstring const& name)
  {
    if (name == module_name_upper || name == module_name_upper_with_ext)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s loaded. Applying hooks.",
                                     module_name_upper.c_str());

      detour_func(module);
    }
  };
  RegisterOnMapCallback(on_map);
  HADESMEM_DETAIL_TRACE_FORMAT_W(L"Registered OnMap for %s.",
                                 module_name_upper.c_str());

  auto const on_unmap = [=](HMODULE module)
  {
    auto const d3d10_mod = get_module_func();
    auto const d3d10_mod_beg = d3d10_mod.first;
    void* const d3d10_mod_end =
      static_cast<std::uint8_t*>(d3d10_mod.first) + d3d10_mod.second;
    if (module >= d3d10_mod_beg && module < d3d10_mod_end)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s unloaded. Removing hooks.",
                                     module_name_upper.c_str());

      // Detach instead of remove hooks because when we get the notification the
      // memory region is already gone.
      undetour_func(false);
    }
  };
  RegisterOnUnmapCallback(on_unmap);
  HADESMEM_DETAIL_TRACE_FORMAT_W(L"Registered OnUnmap for %s.",
                                 module_name_upper.c_str());
}
}
}