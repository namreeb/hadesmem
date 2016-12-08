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

// TODO: Add some sort of 'Update' or 'Rehook' function for use on module
// load/unload.

// TODO: Support 'stealth' IAT hooking where we redirect to code inside the
// module which will raise an exception.

// TODO: We should not patch our own module (or also plugins etc in the case of
// Cerberus, so we need some way to provide a whitelist).

// TODO: Add API set schema support? E.g. We want to avoid something like this:
// http://bit.ly/1Lu4asC

namespace hadesmem
{
// WARNING! Don't use this, still under development.
// TODO: Implement this properly.
template <typename TargetFuncT, typename ContextT = void*> class PatchIat
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
           ContextT context = ContextT())
    : process_{process},
      module_(detail::ToUpperOrdinal(module)),
      function_(function),
      detour_{detour},
      context_(std::move(context))
  {
    if (process.GetId() != ::GetCurrentProcessId())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"PatchIat only supported on local process."});
    }

    hadesmem::ModuleList const modules{process};
    for (auto const& m : modules)
    {
      HookModule(m);
    }
  }

  explicit PatchIat(Process const&& process,
                    std::wstring const& module,
                    std::string const& function,
                    DetourFuncT const& detour,
                    ContextT context = ContextT()) = delete;

  PatchIat(PatchIat const& other) = delete;

  PatchIat& operator=(PatchIat const& other) = delete;

  PatchIat(PatchIat&& other)
    : process_{std::move(other.process_)},
      module_{std::move(other.module_)},
      function_{std::move(other.function_)},
      detour_{std::move(other.detour_)},
      context_(std::move(other.context_)),
      eat_hook_{std::move(other.eat_hook_)},
      iat_hooks_{std::move(other.iat_hooks_)},
  {
  }

  PatchIat& operator=(PatchIat&& other)
  {
    RemoveUnchecked();

    process_ = std::move(other.process_);
    module_ = std::move(other.module_);
    function_ = std::move(other.function_);
    detour_ = std::move(other.detour_);
    context_ = std::move(other.context_);
    eat_hook_ = std::move(other.eat_hook_);
    iat_hooks_ = std::move(other.iat_hooks_);

    return *this;
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

  void RemoveUnchecked() noexcept
  {
    eat_hook_->RemoveUnchecked();

    for (auto& iat_hook : iat_hooks_)
    {
      if (iat_hook.second)
      {
        iat_hook.second->RemoveUnchecked();
      }
    }
  }

private:
  void HookModule(Module const& m)
  {
    hadesmem::PeFile const pe_file{
      process_, m.GetHandle(), hadesmem::PeFileType::kImage, 0};

    auto const cur_mod_name = detail::ToUpperOrdinal(m.GetName());
    if (cur_mod_name == module_)
    {
      HookModuleExports(pe_file);
    }

    HookModuleImports(pe_file);
  }

  void HookModuleExports(PeFile const& pe_file)
  {
    hadesmem::ExportList exports{process_, pe_file};
    for (auto& e : exports)
    {
      if (function_ != e.GetName())
      {
        continue;
      }

      if (e.IsForwarded())
      {
        // TODO: Handle forwarded exports correctly.
        HADESMEM_DETAIL_TRACE_FORMAT_A(
          "WARNING! Unhandled forwarded export with forwarder [%s].",
          e.GetForwarder());
        HADESMEM_DETAIL_ASSERT(false);
      }

      HADESMEM_DETAIL_ASSERT(!eat_hook_);

      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Got export at [%p] with value [%p].", e.GetRvaPtr(), e.GetVa());

      eat_hook_ = std::make_unique<PatchFuncRva<TargetFuncT, ContextT>>(
        process_, pe_file.GetBase(), e.GetRvaPtr(), detour_, context_);
    }
  }

  void HookModuleImports(PeFile const& pe_file)
  {
    hadesmem::ImportDirList const import_dirs{process_, pe_file};
    for (auto const& id : import_dirs)
    {
      // TODO: Handle forwarded exports here also? i.e. Hook both things that
      // import via the forwarder and also hook the real implementation.

      auto const imp_mod_name =
        detail::ToUpperOrdinal(detail::MultiByteToWideChar(id.GetName()));
      if (imp_mod_name != module_)
      {
        continue;
      }

      HookModuleImportDir(pe_file, id);
    }
  }

  void HookModuleImportDir(PeFile const& pe_file, hadesmem::ImportDir const& id)
  {
    hadesmem::ImportThunkList import_thunks{
      process_, pe_file, id.GetFirstThunk()};
    hadesmem::ImportThunkList orig_import_thunks{
      process_, pe_file, id.GetOriginalFirstThunk()};
    for (auto it = std::begin(import_thunks),
              oit = std::begin(orig_import_thunks);
         it != std::end(import_thunks) && oit != std::end(orig_import_thunks);
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

      auto& iat_hook = iat_hooks_[pe_file.GetBase()];
      HADESMEM_DETAIL_ASSERT(!iat_hook);
      auto const func_ptr =
        reinterpret_cast<TargetFuncRawT*>(it->GetFunctionPtr());
      iat_hook = std::make_unique<PatchFuncPtr<TargetFuncT, ContextT>>(
        process_, func_ptr, detour_, context_);
    }
  }

  Process process_;
  std::wstring module_{};
  std::string function_{};
  DetourFuncT detour_{};
  ContextT context_;
  std::unique_ptr<PatchDetourBase> eat_hook_;
  std::map<void*, std::unique_ptr<PatchDetourBase>> iat_hooks_{};
};
}
