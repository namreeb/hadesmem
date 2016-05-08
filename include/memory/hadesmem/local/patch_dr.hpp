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

// TODO: Actually implement this properly.

// TODO: Add context support.

// TODO: Add R/W BP support.

// TODO: Similar to IAT hook todo, add some sort of 'Update' or 'Rehook'
// function for use on thread create/exit.

namespace hadesmem
{
// DANGER DANGER WILL ROBINSON
// This currently has some serious limitations. Notably:
//   Not even close to 'production' quality. Full of subtle bugs, gaps, etc.
//   Can only hook the 'current' thread.
//   Can only set one hook per thread.
//   No validation. e.g. Lets you orphan an existing hook by setting a new one.
//   Stomps over other things which may be using the debug registers.
//   Stomps over other types of VEH hooks (e.g. will stomp over an INT3 hook
//     on the same address).
//   Not handling TID reuse or invalidation.
//   Other bad things. Seriously, my head hurts from thinking of all the corner
//     cases.
template <typename TargetFuncT> class PatchDr : public PatchVeh<TargetFuncT>
{
public:
  template <typename TargetFuncT, typename DetourFuncT>
  explicit PatchDr(Process const& process,
                   TargetFuncT target,
                   DetourFuncT detour)
    : PatchVeh{process, target, detour}
  {
  }

  template <typename TargetFuncT, typename DetourFuncT>
  explicit PatchDr(Process const&& process,
                   TargetFuncT target,
                   DetourFuncT detour) = delete;

  PatchDr(PatchDr const& other) = delete;

  PatchDr& operator=(PatchDr const& other) = delete;

  PatchDr(PatchDr&& other) : PatchVeh{std::move(other)}
  {
  }

  PatchDr& operator=(PatchDr&& other)
  {
    PatchVeh::operator=(std::move(other));
    return *this;
  }

protected:
  virtual std::size_t GetPatchSize() const override
  {
    // The patch size is actually zero, but we need to pretend that we've patch
    // something so we can generate the trampoline to jump over it.
    return 1;
  }

  virtual void WritePatch() override
  {
    hadesmem::detail::AcquireSRWLock const lock(
      &GetSrwLock(), hadesmem::detail::SRWLockType::Exclusive);

    auto& veh_hooks = GetVehHooks();

    HADESMEM_DETAIL_ASSERT(veh_hooks.find(target_) == std::end(veh_hooks));
    veh_hooks[target_] = this;

    auto const veh_cleanup_hook = [&]() {
      auto const veh_hooks_removed = veh_hooks.erase(target_);
      (void)veh_hooks_removed;
      HADESMEM_DETAIL_ASSERT(veh_hooks_removed);
    };
    auto scope_veh_cleanup_hook =
      hadesmem::detail::MakeScopeWarden(veh_cleanup_hook);

    HADESMEM_DETAIL_TRACE_A("Setting DR hook.");

    auto& dr_hooks = GetDrHooks();
    auto const thread_id = ::GetCurrentThreadId();
    HADESMEM_DETAIL_ASSERT(dr_hooks.find(thread_id) == std::end(dr_hooks));

    Thread const thread(thread_id);
    auto context = GetThreadContext(thread, CONTEXT_DEBUG_REGISTERS);

    std::uint32_t dr_index = static_cast<std::uint32_t>(-1);
    for (std::uint32_t i = 0; i < 4; ++i)
    {
      // Check whether the DR is available according to the control register
      bool const control_available = !(context.Dr7 & (1ULL << (i * 2)));
      // Check whether the DR is zero. Pobably not actually necessary, but
      // it's a nice additional sanity check. This may require a
      // user-controlable flag in future though if the code being hooked is
      // 'hostile'.
      bool const dr_available = !(&context.Dr0)[i];
      if (control_available && dr_available)
      {
        dr_index = i;
        break;
      }
    }

    if (dr_index == static_cast<std::uint32_t>(-1))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"No free debug registers."});
    }

    dr_hooks[::GetCurrentThreadId()] = dr_index;

    auto const dr_cleanup_hook = [&]() {
      auto const dr_hooks_removed = dr_hooks.erase(::GetCurrentThreadId());
      (void)dr_hooks_removed;
      HADESMEM_DETAIL_ASSERT(dr_hooks_removed);
    };
    auto scope_dr_cleanup_hook =
      hadesmem::detail::MakeScopeWarden(dr_cleanup_hook);

    (&context.Dr0)[dr_index] = reinterpret_cast<std::uintptr_t>(target_);
    // Set appropriate L0-L3 flag
    context.Dr7 |= static_cast<std::uintptr_t>(1ULL << (dr_index * 2));
    // Set appropriate RW0-RW3 field (Execution)
    std::uintptr_t break_type = 0;
    context.Dr7 |= (break_type << (16 + 4 * dr_index));
    // Set appropriate LEN0-LEN3 field (1 byte)
    std::uintptr_t break_len = 0;
    context.Dr7 |= (break_len << (18 + 4 * dr_index));
    // Set LE flag
    std::uintptr_t local_enable = 1 << 8;
    context.Dr7 |= local_enable;

    SetThreadContext(thread, context);

    scope_veh_cleanup_hook.Dismiss();
    scope_dr_cleanup_hook.Dismiss();
  }

  virtual void RemovePatch() override
  {
    hadesmem::detail::AcquireSRWLock const lock(
      &GetSrwLock(), hadesmem::detail::SRWLockType::Exclusive);

    HADESMEM_DETAIL_TRACE_A("Unsetting DR hook.");

    auto& dr_hooks = GetDrHooks();
    auto const thread_id = ::GetCurrentThreadId();
    auto const iter = dr_hooks.find(thread_id);
    HADESMEM_DETAIL_ASSERT(iter != std::end(dr_hooks));
    auto const dr_index = iter->second;

    Thread const thread(thread_id);
    auto context = GetThreadContext(thread, CONTEXT_DEBUG_REGISTERS);

    // Clear the appropriate DR
    *(&context.Dr0 + dr_index) = 0;
    // Clear appropriate L0-L3 flag
    context.Dr7 &= ~static_cast<std::uintptr_t>(1ULL << (dr_index * 2));

    SetThreadContext(thread, context);

    auto const dr_hooks_removed = dr_hooks.erase(thread_id);
    (void)dr_hooks_removed;
    HADESMEM_DETAIL_ASSERT(dr_hooks_removed);

    auto& veh_hooks = GetVehHooks();
    auto const veh_hooks_removed = veh_hooks.erase(target_);
    (void)veh_hooks_removed;
    HADESMEM_DETAIL_ASSERT(veh_hooks_removed);
  }

  virtual bool CanHookChainImpl() const noexcept override
  {
    return false;
  }
};
}
