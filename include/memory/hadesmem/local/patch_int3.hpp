// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <atomic>
#include <climits>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <udis86.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/alloc.hpp>
#include <hadesmem/detail/alias_cast.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/patch_detour_stub.hpp>
#include <hadesmem/detail/patcher_aux.hpp>
#include <hadesmem/detail/scope_warden.hpp>
#include <hadesmem/detail/srw_lock.hpp>
#include <hadesmem/detail/thread_aux.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/local/patch_veh.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/thread.hpp>
#include <hadesmem/thread_list.hpp>
#include <hadesmem/thread_helpers.hpp>
#include <hadesmem/write.hpp>

// TODO: Support multiple exception types instead of just INT3 (e.g. invalid
// instruction).

// TODO: Add context support.

namespace hadesmem
{
template <typename TargetFuncT> class PatchInt3 : public PatchVeh<TargetFuncT>
{
public:
  explicit PatchInt3(Process const& process,
                     TargetFuncT target,
                     DetourFuncT const& detour)
    : PatchVeh{process, target, detour}
  {
  }

  template <typename TargetFuncT, typename DetourFuncT>
  explicit PatchInt3(Process const&& process,
                     TargetFuncT target,
                     DetourFuncT const& detour) = delete;

  PatchInt3(PatchInt3 const& other) = delete;

  PatchInt3& operator=(PatchInt3 const& other) = delete;

  PatchInt3(PatchInt3&& other) : PatchVeh{std::move(other)}
  {
  }

  PatchInt3& operator=(PatchInt3&& other)
  {
    PatchVeh::operator=(std::move(other));
    return *this;
  }

protected:
  virtual std::size_t GetPatchSize() const override
  {
    // 0xCC
    return 1;
  }

  virtual void WritePatch() override
  {
    auto& veh_hooks = GetVehHooks();

    {
      hadesmem::detail::AcquireSRWLock const lock(
        &GetSrwLock(), hadesmem::detail::SRWLockType::Exclusive);

      HADESMEM_DETAIL_ASSERT(veh_hooks.find(target_) == std::end(veh_hooks));
      veh_hooks[target_] = this;
    }

    auto const cleanup_hook = [&]() { veh_hooks.erase(target_); };
    auto scope_cleanup_hook = hadesmem::detail::MakeScopeWarden(cleanup_hook);

    HADESMEM_DETAIL_TRACE_A("Writing breakpoint.");

    std::vector<std::uint8_t> const buf = {0xCC};
    WriteVector(process_, target_, buf);

    scope_cleanup_hook.Dismiss();
  }

  virtual void RemovePatch() override
  {
    HADESMEM_DETAIL_TRACE_A("Restoring original bytes.");

    WriteVector(process_, target_, orig_);

    {
      hadesmem::detail::AcquireSRWLock const lock(
        &GetSrwLock(), hadesmem::detail::SRWLockType::Exclusive);

      auto& veh_hooks = GetVehHooks();
      veh_hooks.erase(target_);
    }
  }

  virtual bool CanHookChainImpl() const noexcept override
  {
    return false;
  }
};
}
