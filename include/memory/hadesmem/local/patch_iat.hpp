// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <vector>

#include <windows.h>

#include <hadesmem/alloc.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/local/patch_detour_base.hpp>
#include <hadesmem/local/patch_func_ptr.hpp>
#include <hadesmem/local/patch_func_rva.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/import_dir_list.hpp>
#include <hadesmem/pelib/import_thunk.hpp>
#include <hadesmem/pelib/import_thunk_list.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/pelib/export_list.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{
template <typename TargetFuncT> class PatchIat
{
public:
  using TargetFuncRawT =
    std::conditional_t<std::is_member_function_pointer<TargetFuncT>::value,
                       TargetFuncT,
                       std::add_pointer_t<std::remove_pointer_t<TargetFuncT>>>;
  using StubT = detail::PatchDetourStub<TargetFuncT>;
  using DetourFuncRawT = typename StubT::DetourFuncRawT;
  using DetourFuncT = typename StubT::DetourFuncT;

  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<TargetFuncT>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<TargetFuncRawT>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<DetourFuncRawT>::value);

  PatchIat(Process const& process,
           std::wstring const& module,
           std::string const& function,
           DetourFuncT const& detour,
           void* context = nullptr)
    : process_{&process},
      module_(detail::ToUpperOrdinal(module)),
      function_(function),
      detour_{detour},
      context_{context}
  {
    hadesmem::ModuleList const modules{process};
    for (auto const& m : modules)
    {
      hadesmem::PeFile const pe_file{
        process, m.GetHandle(), hadesmem::PeFileType::Image, 0};

      auto const cur_mod_name = detail::ToUpperOrdinal(m.GetName());
      if (cur_mod_name == module_)
      {
        hadesmem::ExportList exports{process, pe_file};
        for (auto& e : exports)
        {
          // TODO: Handle forwarded exports correctly (i.e. follow the forwarder
          // and hook the 'real' export).

          if (function_ != e.GetName())
          {
            continue;
          }

          HADESMEM_DETAIL_ASSERT(!eat_hook_);

          HADESMEM_DETAIL_TRACE_FORMAT_A(
            "Got export at [%p] with value [%p].", e.GetRvaPtr(), e.GetVa());

          eat_hook_ = std::make_unique<PatchFuncRva<TargetFuncT>>(
            process, m.GetHandle(), e.GetRvaPtr(), detour_, context_);
        }
      }

      hadesmem::ImportDirList const import_dirs{process, pe_file};
      for (auto const& id : import_dirs)
      {
        auto const imp_mod_name =
          detail::ToUpperOrdinal(detail::MultiByteToWideChar(id.GetName()));
        if (imp_mod_name != module_)
        {
          continue;
        }

        hadesmem::ImportThunkList import_thunks{
          process, pe_file, id.GetFirstThunk()};
        hadesmem::ImportThunkList orig_import_thunks{
          process, pe_file, id.GetOriginalFirstThunk()};
        for (auto it = std::begin(import_thunks),
                  oit = std::begin(orig_import_thunks);
             it != std::end(import_thunks) &&
               oit != std::end(orig_import_thunks);
             ++it, ++oit)
        {
          if (function_ != oit->GetName())
          {
            continue;
          }

          HADESMEM_DETAIL_TRACE_FORMAT_A(
            "Got import thunk at [%p] with value [%p].",
            it->GetFunctionPtr(),
            reinterpret_cast<void const*>(it->GetFunction()));

          auto& iat_hook = iat_hooks_[m.GetHandle()];
          HADESMEM_DETAIL_ASSERT(!iat_hook);
          auto const func_ptr =
            reinterpret_cast<TargetFuncRawT*>(it->GetFunctionPtr());
          iat_hook = std::make_unique<PatchFuncPtr<TargetFuncT>>(
            process, func_ptr, detour_, context_);
        }
      }
    }
  }

  ~PatchIat()
  {
    RemoveUnchecked();
  }

  void Apply()
  {
    if (eat_hook_)
    {
      eat_hook_->Apply();
    }

    for (auto& iat_hook : iat_hooks_)
    {
      if (iat_hook.second)
      {
        iat_hook.second->Apply();
      }
    }
  }

  void Remove()
  {
    if (eat_hook_)
    {
      eat_hook_->Remove();
    }

    for (auto& iat_hook : iat_hooks_)
    {
      if (iat_hook.second)
      {
        iat_hook.second->Remove();
      }
    }
  }

private:
  void RemoveUnchecked() HADESMEM_DETAIL_NOEXCEPT
  {
    if (eat_hook_)
    {
      try
      {
        eat_hook_->Remove();
      }
      catch (...)
      {
        // WARNING: Patch may not be removed if Remove fails.
        HADESMEM_DETAIL_TRACE_A(
          boost::current_exception_diagnostic_information().c_str());
        HADESMEM_DETAIL_ASSERT(false);
      }
    }

    for (auto& iat_hook : iat_hooks_)
    {
      if (iat_hook.second)
      {
        try
        {
          iat_hook.second->Remove();
        }
        catch (...)
        {
          // WARNING: Patch may not be removed if Remove fails.
          HADESMEM_DETAIL_TRACE_A(
            boost::current_exception_diagnostic_information().c_str());
          HADESMEM_DETAIL_ASSERT(false);
        }
      }
    }
  }

  Process const* process_;
  std::wstring module_{};
  std::string function_{};
  DetourFuncT detour_{};
  void* context_{};
  std::unique_ptr<PatchDetourBase> eat_hook_;
  std::map<HMODULE, std::unique_ptr<PatchDetourBase>> iat_hooks_{};
};
}
