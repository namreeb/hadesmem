// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "helpers.hpp"

#include "module.hpp"

namespace
{
class HelperImpl : public hadesmem::cerberus::HelperInterface
{
public:
  virtual std::pair<std::size_t, std::size_t> InitializeSupportForModule(
    std::wstring const& module_name,
    std::function<void(HMODULE)> const& detour_func,
    std::function<void(bool)> const& undetour_func,
    std::function<std::pair<void*, SIZE_T>&()> const& get_module_func,
    bool on_map) final
  {
    auto& module = hadesmem::cerberus::GetModuleInterface();

    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Initializing %s support.",
                                   module_name.c_str());

    std::size_t on_map_id = 0;

    auto const module_name_upper =
      hadesmem::detail::ToUpperOrdinal(module_name);
    auto const module_name_upper_with_ext = module_name_upper + L".DLL";

    if (on_map)
    {
      auto const on_map_fn =
        [=](HMODULE mod, std::wstring const& /*path*/, std::wstring const& name)
      {
        if (name == module_name_upper || name == module_name_upper_with_ext)
        {
          HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s mapped. Applying hooks.",
                                         module_name_upper.c_str());

          detour_func(mod);
        }
      };
      on_map_id = module.RegisterOnMap(on_map_fn);
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Registered OnMap for %s.",
                                     module_name_upper.c_str());
    }
    else
    {
      auto const on_load_fn = [=](HMODULE mod,
                                  PCWSTR /*path*/,
                                  PULONG /*flags*/,
                                  std::wstring const& /*full_name*/,
                                  std::wstring const& name)
      {
        if (name == module_name_upper || name == module_name_upper_with_ext)
        {
          HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s loaded. Applying hooks.",
                                         module_name_upper.c_str());

          detour_func(mod);
        }
      };
      on_map_id = module.RegisterOnLoad(on_load_fn);
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Registered OnLoad for %s.",
                                     module_name_upper.c_str());
    }

    auto const on_unmap_fn = [=](HMODULE mod)
    {
      auto const module_data = get_module_func();
      auto const module_beg = module_data.first;
      void* const module_end =
        static_cast<std::uint8_t*>(module_data.first) + module_data.second;
      if (mod >= module_beg && mod < module_end)
      {
        HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s unmapped. Removing hooks.",
                                       module_name_upper.c_str());

        // Detach instead of remove hooks because when we get the notification
        // the memory region is already gone.
        undetour_func(false);
      }
    };
    auto const on_unmap_id = module.RegisterOnUnmap(on_unmap_fn);
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Registered OnUnmap for %s.",
                                   module_name_upper.c_str());

    return {on_map_id, on_unmap_id};
  }

  virtual bool CommonDetourModule(hadesmem::Process const& process,
                                  std::wstring const& name,
                                  HMODULE& base,
                                  std::pair<void*, SIZE_T>& detoured_mod) final
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Module: [%s].", name.c_str());

    if (detoured_mod.first)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s already detoured.", name.c_str());
      return false;
    }

    if (!base)
    {
      base = ::GetModuleHandleW(name.c_str());
    }

    if (!base)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Failed to find %s module.",
                                     name.c_str());
      return false;
    }

    detoured_mod =
      std::make_pair(base, hadesmem::detail::GetModuleRegionSize(process, base));

    HADESMEM_DETAIL_TRACE_FORMAT_W(
      L"Base: [%p]. Size: [%IX].", detoured_mod.first, detoured_mod.second);

    return true;
  }

  virtual bool
    CommonUndetourModule(std::wstring const& name,
                         std::pair<void*, SIZE_T>& detoured_mod) final
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"Module: [%s].", name.c_str());

    (void)name;
    if (!detoured_mod.first)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s not detoured.", name.c_str());
      return false;
    }

    return true;
  }
};
}

namespace hadesmem
{
namespace cerberus
{
HelperInterface& GetHelperInterface() noexcept
{
  static HelperImpl helper_impl;
  return helper_impl;
}
}
}
